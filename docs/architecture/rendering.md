# Rendering

[Русская версия](rendering.ru.md)

Cameras, views and how the scene reaches the renderer. The stages of the
render frame are in [the frame pipeline](frame.md).

Status marks: see [README](README.md).

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

## Editor overlay

The editor overlay (grid, selection outline) leaves engine frame data when
the render hardware interface arrives, before step 5.
