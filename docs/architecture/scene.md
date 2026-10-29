# Scene architecture: nodes, components, resources

[Русская версия](scene.ru.md)

This is the contract for everything that lives in a scene: what is a node,
what is a component, how types are registered, the lifecycle, and how scripts,
the editor and hot reload rely on it. It changes together with the code.

Status marks: **[done]** implemented, **[next]** the next stage of work
(before C++ scripts), **[planned]** later, with the step it belongs to.

## Design decisions

1. **Games add components, not node types.** Node types come only from the
   engine. A game still creates nodes of engine types (`Node`, `Node3D`,
   `Camera3D`, ...) in code and attaches its own components to them. The type
   tree of nodes never changes when a game module is reloaded.
2. **A script is a component.** It cannot exist in the world on its own, only
   attached to a node. Every script class is its own component type, in C++,
   in Luau and in visual scripts alike.
3. **One component of a type per node by default.** A type opts in to
   repetition with `.allowMultiple()`.
4. **The core does not assume 3D.** Only 3D is implemented in the MVP, 2D comes
   after it, as a separate family next to 3D rather than inside it.
5. **Rendering, physics and audio are components on a node, not node types.**
   A mesh is `MeshRenderer` on a `Node3D`; a 2D sprite will be `SpriteRenderer`
   on a `Node2D`.
6. **Scripting languages:** C++ first (MVP), then Luau after the MVP, then
   visual scripts after 2D.

## Core concepts

| | What it is | Who defines types | Examples |
|---|---|---|---|
| **Node** | An object of the scene with its own role, one row in the scene tree | engine only | `Node`, `Node3D`, `Camera3D`, lights, UI controls |
| **Component** | An ability or behaviour attached to a node | engine and game | `MeshRenderer`, `RigidBody`, `BoxCollider`, `AudioSource`, scripts |
| **Resource** | A project file with a stable ID | engine (formats) | mesh, material, shader, texture, sound, scene, script source |

How to choose:

- The object makes no sense without it and is visible as a thing of its own in
  the tree (a camera, a light): a **node**.
- It is an addition that fits many different objects: a **component**.
- It is data shared by several objects: a **resource**, and a component refers
  to it.

## Node hierarchy

```
Node                     no position: a group, a root, a holder of scripts
├── Node3D               built-in Transform3D (cannot be removed)
│   ├── Camera3D
│   └── DirectionalLight3D, PointLight3D, SpotLight3D      [planned, step 5]
├── Control              built-in RectTransform, anchors   [planned, step 8]
│   └── Label, Button, ...
└── Node2D               built-in Transform2D              [planned, after MVP]
    └── Camera2D, ...
```

- `Node2D` does not derive from `Node3D` and vice versa: the transforms have
  different meaning. They share only `Node`, the tree, the lifecycle and the
  editor infrastructure.
- A node class holds the properties of its role (field of view, light colour)
  and registers itself in scene systems in `onEnterScene` / `onExitScene`.
  **No game logic in node classes**: per-frame work of engine nodes belongs to
  their system. `onStart` / `onUpdate` / `onStop` are for components only
  **[done]**.

## Components

### Registration and rules [done]

```cpp
registry.registerComponent<MeshRenderer>()
    .requiresNode<Node3D>()               // which nodes it may live on
    .property("mesh", &MeshRenderer::mesh, &MeshRenderer::setMesh, PropertyHint::Mesh);

registry.registerComponent<BoxCollider>()
    .requiresNode<Node3D>()
    .requiresComponent<RigidBody>();      // needs a RigidBody on the same node

registry.registerComponent<AudioSource>()
    .requiresNode<Node3D>()
    .allowMultiple();
```

- `requiresNode<T>()`: the owner must be `T` or derived from it.
- `requiresComponent<T>()`: the same node must have a component of type `T`
  or derived from it. The editor adds it together with the dependent one, in
  one undo step. A required component cannot be removed while something
  depends on it: its header has no close button and the tooltip names the
  dependents. Removing the dependents too is never implicit.
- `allowMultiple()`: several components of this type on one node. Each
  instance is still checked against `requiresNode` and `requiresComponent`.
- Rules are inherited: the rules of a base component type apply to derived
  ones.
- The base type comes from `YQLORA_TYPE(Class, Base)`, not from a second
  template argument of the registration, and registration does not compile
  if the class does not derive from it **[done]**.

`requires` is a C++20 keyword, hence `requiresNode` / `requiresComponent`.

### Who enforces the rules

The checks themselves are free functions in `ComponentRules.hpp` (`canAttach`,
`missingRequirements`, `dependentsOf`); everyone below calls them.

- The editor, through `EditorSession` **[done]**.
- The scene loader: a component on a node it does not fit is skipped with an
  error in the log; a missing required component is added with a warning
  **[done]**.
- `Node::addComponent` itself, for a node in a scene: it adds missing required
  components first, like `AddComponent` in Unity, and a broken rule is an error
  of the program, so game code cannot bypass them either **[done]**.

### Built-in components

A spatial node creates its transform in the constructor (`Transform3D` for
`Node3D`). It cannot be removed and is not offered in "Add component".

## Scripts in C++ [planned, step 2]

```cpp
class Player : public yqlora::Script {
public:
    YQLORA_TYPE(Player, Script)

    float speed = 5.0f;

protected:
    void onUpdate(float deltaSeconds) override;
};

// In the game module:
registry.registerComponent<Player>().property("speed", &Player::speed);
```

- `Script` is a `Component` with the full lifecycle and access to the engine
  context. Scripts are registered in the game module (`TypeRegistry::ModuleScope`)
  and allow several per node by default.
- A script source file is a resource; the scene refers to the script by its
  component type name.
- An exception thrown from a lifecycle call is caught: the component is
  disabled and the error goes to the log. The game loop goes on. A memory
  error in native code cannot be caught; that is the price of C++ scripts.

## Scripts in Luau [planned, after MVP]

[Luau](https://luau.org) (MIT): Lua 5.1 with gradual typing, a built-in
sandbox and a language server.

- Every `.luau` script registers **its own component type** in the registry,
  named after the script, with the properties declared in the script. All of
  them create instances of one C++ class, `LuauScript`, bound to the script
  resource. So the registry must accept types and properties defined at run
  time, not only from C++ templates **[next]**: properties are already
  type-erased (`std::function` over `void*`), the builder is not.
- Bindings are generated from the registry: properties plus **registered
  methods** of engine types **[next]**. One list serves Luau, the inspector and
  the API documentation. The same list produces type definition files for
  luau-lsp, which gives completion and type checking in an editor.
- Sandbox: Luau's own (`luaL_sandbox`: read-only globals, no `io` / `os`
  access), an interrupt callback against endless loops, an allocator with a
  memory limit.
- A Luau error disables the component and goes to the log, like a C++
  exception.
- Luau scripts are their own type module (`luau`), reloaded like `game.dll`.

## Visual scripts [planned, after 2D]

For those who do not write code: a script is built from blocks connected by
links, then attached to a node and works like a C++ or Luau script.

- A graph is a resource (a file of the project): blocks, their pins and the
  links between them. Execution links set the order of actions, data links
  carry values.
- Every graph registers **its own component type**, like a Luau script, with
  the properties declared in the graph. All of them create instances of one
  C++ class, `VisualScript`, bound to the graph resource.
- Entry blocks are the lifecycle calls (`onStart`, `onUpdate`, `onStop`) and
  later events (input, collisions).
- The block palette is generated from the same registry list as the Luau
  bindings: properties and registered methods of engine and game types. A new
  method of a type becomes a block without extra work.
- The engine interprets the graph. A limit on steps per call guards against
  endless loops; an error disables the component and goes to the log, like a
  C++ exception.
- Visual scripts are their own type module (`visual`), reloaded when a graph
  is saved.
- The graph editor is a panel of the editor; editing logic lives apart from
  ImGui, like the rest of the editor.

## Engine context

Three levels: what the process shares, what every scene owns, what a component
sees.

```
Owner (game / editor / test)
 └─ EngineServices     shared by the process: input, project, type registry, scene requests
     └─ Scene          owned by every scene
         ├─ GameTime        game time, time scale, fixed step
         ├─ systems         render, physics, audio... (pluggable)
         ├─ phase lists     components to call in fixedUpdate / update / lateUpdate
         └─ node tree
             └─ Component → owner()->scene() → services(), time(), system<T>()
```

### Engine services [done]

```cpp
class EngineServices {
public:
    const Input& input() const;          // raw state + actions from the project settings
    const Project* project() const;      // settings and resources; nullptr when there is none
    const TypeRegistry& registry() const;
    SceneRequests& scenes();             // "load a level": done between frames  [planned, step 9]
};
```

- A scene gets its services when it is created: `Scene(const EngineServices&)`.
  Tests and empty scenes get services with nothing pressed and an empty
  project, so no code checks for `nullptr`.
- No `Application::instance()`. Log and translation stay global by design.
- The registry in the services is what lets `Node::addComponent` check the
  component rules itself and what tells the scene the lifecycle phases of a
  type.
- In the editor the Play copy sees input only while the viewport is focused,
  so keys typed in the inspector do not move the player.

### Game time [done]

```cpp
class GameTime {
public:
    double elapsed() const;          // since the game started, scaled
    float delta() const;             // this frame, scaled
    float unscaledDelta() const;     // real step: editor, HUD animations
    float fixedStep() const;         // from the Time section of the project settings
    float scale() const;
    void setScale(float scale);      // 0 stops the game, 0.5 is slow motion
    std::uint64_t frame() const;
};
```

Every scene has its own; the Play copy starts at zero and pause stops it. The
real frame length comes from `FrameClock`, game time is derived from it.

### Scene systems [done]

The main extension point: a module adds a system and its components; `Scene`,
the frame loop and the application do not change.

```cpp
class PhysicsWorld : public SceneSystem {
public:
    YQLORA_TYPE(PhysicsWorld, SceneSystem)
    explicit PhysicsWorld(Scene& scene);

    void fixedUpdate(float step) override;     // physics steps here
    // also update, lateUpdate (after scripts: render list, audio)
    // and updateTransforms (after the simulation, before extract)
};

// A module registers the type once; every new scene gets an instance:
registry.registerSystem<PhysicsWorld>();
scene.system<PhysicsWorld>()                    // a component registers its body in onEnterScene
```

- Systems are types of the registry (`TypeKind::System`), so they belong to a
  module and leave with it like components. A scene creates one instance of
  every system type when it is constructed; on a reload of the game module the
  scene is recreated together with its tree **[planned, step 3]**.
- Systems are called in the order of registration. An explicit order comes
  with the first pair of systems that needs one.
- The edited scene in the editor has its systems too (bodies exist, colliders
  can be drawn), but only the Play copy is stepped.
- `TransformSystem3D` is the first system: it keeps the world matrices of 3D
  nodes fresh, see the frame pipeline.

### One frame [done]

The order lives in one place, `Scene::advance`:

```
scene.advance(realDelta):
    time.advance(realDelta)                       scale, fixed-step accumulator
    while a fixed step is accumulated:
        systems.fixedUpdate, then components.onFixedUpdate
    components.onUpdate
    systems.update
    components.onLateUpdate
    systems.lateUpdate
    apply deferred tree changes                   destroy, change of parent
```

The game (`Application`) calls `advance` every frame, the editor's `PlayMode`
calls it for the Play copy when not paused, tests call it directly.

## Frame pipeline

The whole engine frame, not only the scene part. Each stage has one owner;
the order lives in one frame loop class that knows only the order **[next]**.

```
1. Begin       FrameClock: frame length
2. Input       window events -> raw input -> actions from the project settings
3. Simulate    scene.advance: fixed steps (physics) -> update -> late update
               -> deferred tree changes
4. Transforms  world matrices of changed nodes, one pass with a dirty flag
5. Extract     the render system takes a snapshot of the scene: views (cameras),
               what to draw, lights. Nothing reads the scene after this.
6. Render      prepare: culling by visibility and layer mask per view, sorting
               (opaque front to back, transparent back to front)
               graph: render passes (shadows, opaque, sky, transparent,
               post-processing, HUD) through the RHI
7. Interface   ImGui of the editor and tools
8. Present
```

- **The snapshot from Extract** is all the renderer reads; it never touches
  the scene. Later the renderer can move to its own thread (the game computes
  frame N+1 while frame N is drawn) without changing game code. Extract is
  `extractFrame(scene, ...)`: the views of the render system's cameras and
  what to draw, with layers **[done]**.
- **World matrices** are cached with a dirty flag: a change of a transform
  marks the node and its branch, a read recomputes only what is stale
  **[done]**. `TransformSystem3D` queues the top changed node of every branch
  and refreshes them in one pass between simulation and extract
  (`scene.updateTransforms()`), so extract reads ready values **[done]**.
- **The render graph**: a pass declares what it reads and writes, the graph
  orders the passes. A new effect is one more pass, not a change of the
  renderer. The editor adds its own passes (grid, selection outline) to the
  graph, which removes `FrameData::editor`. Culling and sorting come with it
  **[planned, RHI]**.
- **The render thread** comes when it is needed; the snapshot from Extract
  already allows it **[planned, later]**.

## Lifecycle

| Call | Where | What for |
|---|---|---|
| constructor | anywhere | defaults; no scene yet |
| `onEnterScene` / `onExitScene` | editor and game | register in scene systems (physics, audio, lights) |
| `onEnable` / `onDisable` | game only | the component starts or stops taking part: enabled and its node active in hierarchy **[done]** |
| `onStart` | game only | once, before the first phase call of the component |
| `onFixedUpdate(step)` | game only | physics, fixed step of game time **[done]** |
| `onUpdate(delta)` | game only | per-frame logic |
| `onLateUpdate(delta)` | game only | after all updates: follow cameras **[done]** |
| `onStop` | game only | release what `onStart` took |

### Phase lists instead of a tree walk [done]

- A scene keeps one list per phase (fixed update, update, late update) and
  calls only the components in it. A component gets into a list only if its
  type overrides that phase; the registry finds this out when the type is
  registered (`PhaseDetector` compares the declaring class of `onUpdate` and
  the others; `YQLORA_TYPE` makes it a friend of the class). No tree walk, no
  empty virtual calls.
- Order inside a phase: the **execution order** of the type (default 0, lower
  first), then the order in which components entered the list. A type sets its
  default in code (`.executionOrder(-100)`); the project settings can override
  it per type, like Script Execution Order in Unity (see
  [project settings](project-settings.md)). One rule: the setting if present,
  else the default from code.
- A component added during a phase is called from the next phase on; a removed
  one is skipped at once and dropped from the list after the phase, so the
  lists never change under an iteration.

### Active and enabled [done]

As in Unity:

- A node has `active` (its own flag) and `activeInHierarchy()` (it and all its
  parents are active). An inactive node switches off its whole branch:
  components get `onDisable`, leave the phase lists, renderers are not drawn,
  bodies leave the physics world.
- A component has `enabled`. It takes part in the game only while enabled and
  its node is active in hierarchy; `onEnable` / `onDisable` mark the change.
- There is no separate `visible` flag: hiding a mesh is disabling its
  `MeshRenderer`, like `Renderer.enabled` in Unity.
- A node has a **layer** (one of 32, names in the project settings). Camera
  masks and physics collisions filter by layers **[done]**; until the layer
  names come, the inspector shows numbers.
- An exception from a lifecycle call is caught: the component is disabled, the
  error goes to the log, the game goes on.
- `active` and `enabled` are ordinary registry properties (of `Node` and of
  the `Component` root type): the scene file, the inspector, undo and later
  the script bindings get them without extra code.
- `onEnable` comes before `onStart` and before the first phase after the
  component is enabled again; `onDisable` comes at once on disabling and
  before `onStop`. Re-enabling does not repeat `onStart`.
- A builtin component (`Transform3D`) cannot be disabled, as it cannot be
  removed: it has no `enabled` property.

### Deferred tree changes [done]

Destroying a node and changing its parent during a frame are applied at the
end of the frame, so no phase sees a half-changed tree. Nodes created during a
frame take part from the next one.

```cpp
scene.destroy(node);              // the node with its branch, the root too
scene.setParent(node, newParent, KeepWorld::No); // last child of newParent
```

- Outside a frame (editor, loading, tests) both apply at once; `inFrame()`
  tells which case it is.
- The queue refers to nodes by ID: a node gone with a destroyed branch is
  skipped, destroying twice is harmless.
- A move keeps the components running (no `onDisable` / `onEnable`, unless the
  new parent changes `activeInHierarchy`). What happens to the position is
  the choice below.
- A component enabled during a frame, like one of a new node, takes part from
  the next frame.
- `removeChild`, `setRoot` and `takeRoot` during a frame are a programming
  error: they would change the tree under the phases.

### Changing the parent: local or world position

Both are needed: a sword taken into a hand gets a new position relative to
the hand; a wagon hooked to a train must not move in the world.

- **Scripts** say it in every call, there is no default (Unity and Godot have
  opposite ones, any default would surprise someone) **[done]**:

  ```cpp
  scene.setParent(sword, hand, KeepWorld::No);    // local values stay, the node jumps
  scene.setParent(wagon, train, KeepWorld::Yes);  // stays in place in the world
  ```

- **The editor** asks when a node is dragged to another parent in the tree:
  "Keep the world position?" with a "Remember my choice" checkbox. The choice
  is an editor setting (a habit of the person, not a property of the game) and
  can be changed there. Alt while dropping does the opposite once. Undo returns
  both the parent and the old transform. Comes with dragging nodes in the tree
  **[planned, step 4]**.
- Keeping the world position is not always exact: under a parent with
  non-uniform scale and rotation the world matrix has a skew that position,
  rotation and scale cannot hold; the nearest values are taken, as in Unity.
- Nodes without a transform (`Node`) and parents that are not `Node3D` have
  nothing to recompute: the flag changes nothing there.

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
- The editor's own camera (no game camera, or not playing) is drawn as one
  view with the background from the editor theme.
- **Camera preview** in the editor, as in Unity: while a camera is selected, a
  small window in the corner of the viewport shows the picture through it, so
  the camera can be placed by eye. A button on the viewport toggles it, the
  choice is an editor setting **[next]**.
- **Two windows during Play**, as in Unity: "Game" shows the main camera,
  "Scene" keeps the free editor camera, where nodes are selected and changed
  while the game runs. Changes made during Play belong to the Play copy and
  are gone after Stop **[next]**.
- A scene switch requested by a script happens between frames, never in the
  middle of an update **[planned, step 9]**.

## References, state and serialization

- A reference to a node is a `NodeId`, to a resource a `ResourceId`; both are
  property types. A raw pointer to another node or component may be kept only
  between `onEnterScene` and `onExitScene` and must be dropped on exit.
- **Registered properties are saved; everything else is run-time state.** The
  Play copy and hot reload go through serialization, so only properties
  survive them. That is the contract for scripts: whatever must survive a
  reload is a property.
- Serialization restores state, it does not manage lifetimes: subscriptions,
  system registrations and resources are released in `onStop` /
  `onExitScene` before a module is unloaded.

## Hot reload [planned, step 3]

1. Stop the game if it is running.
2. Serialize the scene; take the whole tree out of it, so every component
   gets `onExitScene` and no code of the old module runs afterwards.
3. Destroy the tree, `TypeRegistry::removeModule`, clean the per-type
   component index of the scene (keyed by `std::type_index` today, which would
   point into the unloaded code), unload the library.
4. Load the new library, register its types, load the scene back.

A component whose type is missing (the game failed to compile, a script was
deleted) is kept as a placeholder holding its raw data, shown in the inspector
as missing, and written back unchanged on save. Otherwise saving the scene
would silently drop the data **[planned, step 3]**.

## Editor integration

- The inspector is built from properties and their hints (`PropertyHint`). A
  custom inspector for a type is added only when hints are not enough.
- "Add component" lists only the types that fit the node: `requiresNode`, the
  one-per-node rule and requirements that can be met **[done]**.
- Gizmos for a type live in `editor/ui/gizmos` (like `CameraGizmo`); the engine
  knows nothing about them.
- The editor overlay (grid, selection outline) leaves engine frame data when
  the render hardware interface arrives, before step 5.

## Code layout

Each system is a separate folder with its own type registration;
`registerEngineTypes` only calls them.

```
engine/include/yqlora/scene/     Node, Node3D, Transform3D, Component, Script, Scene
engine/include/yqlora/render/    MeshRenderer, Camera3D, lights
engine/include/yqlora/physics/   RigidBody, colliders, PhysicsWorld      [step 6]
engine/include/yqlora/audio/     AudioSource, AudioListener, AudioWorld  [step 7]
engine/include/yqlora/ui/        Control, Label, Button                  [step 8]
```

Moving `MeshRenderer` and `Camera3D` into `render/` is part of the next stage.
