# Rendering

[Русская версия](rendering.ru.md)

Cameras, views and how the scene reaches the renderer. The stages of the
render frame are in [the frame pipeline](frame.md).

Status marks: see [README](README.md).

## Render pipeline

How a frame gets from the scene to the screen. The renderer reads only the
snapshot made by Extract, never the scene.

### Today [done]

```
 SCENE (simulated, world matrices ready)
   RenderSystem: active cameras sorted by order
   MeshRenderer: enabled components, their mesh (from ResourceCache),
                 colour, node layer
   Node3D: world matrices
        |
        |  extractFrame(scene, aspect, depthRange)        render/FrameBuilder
        v
 FrameData (snapshot, nothing points back into the scene)
   views[]      one per camera: view and projection, clear mode,
                background, culling mask
   drawItems[]  mesh with its ID and revision, world matrix, colour,
                layer, node id
   editor       grid on/off, selected nodes          (editor only)
        |
        |  Renderer::render(frame)          -> window back buffer
        |  Renderer::render(frame, target)  -> RenderTarget (a texture)
        v
 OpenGLRenderer::drawScene                           backends/opengl
   for each view, in camera order:
     clear        colour and depth / depth only / nothing
     MeshPass     draw items whose layer is in the view mask, through
                  the view camera, one fixed directional light
   GridPass       editor only, over the last view
   OutlinePass    editor only, outline of the selected nodes
        |
        v
 UiRenderer (ImGui), then present (swap buffers)
```

- **The game** (`Application`, game mode) does it once per frame in the
  `extract` and `render` stages of `FrameLoop` (see [frame](frame.md)), into
  the window.
- **The editor** has no scene of its own to draw in those stages. Each scene
  window (Viewport, Game, the camera preview) calls `extractFrame` with its
  own aspect and renders into its own `RenderTarget` during the interface
  stage; ImGui then shows the texture. The Viewport adds the editor camera
  view and the overlay (grid, outline).

### With the RHI [planned, RHI]

```
 Extract   -> FrameData (also lights, materials)
 Prepare   -> per view: culling by bounds and layer mask, sorting
              (opaque front to back, transparent back to front)
 Graph     -> passes declare what they read and write, the graph orders them:
              shadows -> opaque -> sky -> transparent -> post-processing -> HUD
              (+ editor passes: grid, outline; FrameData::editor goes away)
 RHI       -> one interface over OpenGL, Vulkan, D3D12
 Present
```

## Meshes and textures [done]

- **A vertex** (`Vertex`): position, normal, `uv0`. `uv0 = (0, 0)` is the top
  left corner of the picture, as in glTF; tangents come with normal maps in
  step 5.
- **A mesh** (`MeshData`): vertices, indices of triangles counter-clockwise
  from outside, submeshes and bounds (AABB). A submesh is a range of indices
  with its own material; its index is the material slot of a `MeshRenderer`
  (step 5), and it keeps the index of the material of the source file. The
  bounds speed up picking, later culling.
- **A texture** (`TextureData`): RGBA one byte per channel, rows from the
  top, and how to read it: sRGB or Linear, filter, wrap, mipmaps (import
  settings). Textures go onto meshes through materials, step 5.
- **GPU copies** are kept by the backend itself, by resource ID and revision
  (`GpuResourceCache`): a `DrawItem` carries the mesh with its ID and
  revision, a copy of another revision is replaced, and a copy not drawn for
  120 frames is released. One template for every backend and kind of
  resource.
- **A texture in the interface**: `Renderer::uiTexture` gives the texture of
  a resource for ImGui (the preview in the inspector) with the colours of
  the file, without sRGB decoding: ImGui draws into a plain buffer, and a
  decoded picture would be darker.

## Cameras and rendering [done]

Prepared for several cameras (for example the player's arms drawn by a second
camera over the world, so they never go into walls); the arms themselves are
not part of this.

- `Camera3D` has an **order** (lower is drawn first), a **culling mask** of
  layers, a **clear mode** (colour and depth, depth only, nothing) and a
  **background** colour for the first mode. The background is the camera's,
  not the scene's: a second camera clearing colour and depth uses its own.
- `RenderSystem` keeps the cameras of the scene (a camera adds itself when
  it enters the scene) and gives those with an active node, sorted by order. The frame data carries a list of views instead of one camera; the
  backend draws them in turn, each with its own clear and only the items whose
  layer is in its mask.
- The **main camera** is the first one in that order, the one the game is
  seen through: `scene.system<RenderSystem>()->mainCamera()`. It is not
  stored anywhere, neither in `Scene` nor in the scene file; to change it,
  change the order or deactivate a camera. A camera that leaves the scene
  drops out of the list by itself.
- **Choosing the main camera in the editor**: the inspector of a camera says
  "Main camera" for the main one and "Inactive" for a camera whose node is
  off; the others get a "Make main" button. It sets the camera's order one
  below the main camera's (`orderToBecomeMain`), one undoable step. There is
  deliberately no "main" flag next to the order: two sources would contradict
  each other (two flagged cameras, none flagged), and the order still decides
  which camera draws over which **[done]**.
- The editor's own camera (no game camera, or not playing) is drawn as one
  view with the background from the editor theme.
- **Camera preview** in the editor, as in Unity: while a camera is selected, a
  small window in the corner of the viewport shows the picture through it, so
  the camera can be placed by eye. A button on the viewport toggles it, the
  choice is an editor setting (`EditorSettings`, a file in the user folder)
  **[done]**.
- **Two windows during Play**, as in Unity: "Game" shows the main camera,
  "Scene" keeps the free editor camera, where nodes are selected and changed
  while the game runs. Changes made during Play belong to the Play copy and
  are gone after Stop **[done]**. The Game window comes forward and takes the
  input on Play; without a game it shows the edited scene through its main
  camera. Mouse position relative to the Game window comes with the HUD
  **[planned, step 8]**.
- A scene switch requested by a script happens between frames, never in the
  middle of an update **[planned, step 9]**.

## Changing the graphics API [done]

The API comes from the Graphics section of the project settings (one per
system) and changes without restarting the application. `Application`
compares it with the running renderer every frame; on a change it rebuilds
only the render part:

```
onRendererLost      owners release what they created through the renderer
                    (the editor: textures of the scene windows)
UiLayer             drops the ImGui drawing of the old API; the ImGui
                    context, windows and layout stay
Renderer            destroyed while its context is alive
Window              recreated only if the new API needs other window flags
                    (OpenGL and Vulkan windows differ in SDL)
Renderer            created for the new API
UiLayer             ImGui drawing for the new API
```

- The scene, the project, the edit history and the editor state live outside
  the renderer, which sees only the frame snapshot, so nothing else notices.
- An API that is not built into this engine, or that the system refuses,
  leaves the current one in place with a message in the log, once.
- Vsync is switched on the running renderer, without a rebuild.
- The same path will recover from a lost device (Vulkan, D3D12)
  **[planned, RHI]**.

## Editor overlay

The editor overlay (grid, selection outline) leaves engine frame data when
the render hardware interface arrives, before step 5.
