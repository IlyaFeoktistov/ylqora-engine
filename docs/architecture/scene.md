# Scene architecture: nodes, components, resources

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
   attached to a node. Every script class is its own component type, in C++
   and in Luau alike.
3. **One component of a type per node by default.** A type opts in to
   repetition with `.allowMultiple()`.
4. **The core does not assume 3D.** Only 3D is implemented in the MVP, 2D comes
   after it, as a separate family next to 3D rather than inside it.
5. **Rendering, physics and audio are components on a node, not node types.**
   A mesh is `MeshRenderer` on a `Node3D`; a 2D sprite will be `SpriteRenderer`
   on a `Node2D`.
6. **Scripting languages:** C++ first (MVP), then Luau after the MVP.

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
- `Node::addComponent` itself, once the scene has the engine context (see
  below), so game code cannot bypass them either **[next]**.

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

## Lifecycle

| Call | Where | What for |
|---|---|---|
| constructor | anywhere | defaults; no scene yet |
| `onEnterScene` / `onExitScene` | editor and game | register in scene systems (physics, audio, lights) |
| `onStart` | game only | before the first frame of the component |
| `onFixedUpdate(dt)` | game only | physics, fixed time step **[next]** |
| `onUpdate(dt)` | game only | per-frame logic |
| `onStop` | game only | release what `onStart` took |

- Order: the tree from the top down; on each node its components in list order.
  On scene entry a node goes before its children, on exit after them.
- A component added during the game gets `onStart` before its first update.
- **Changes of the tree during a frame are deferred [next]**: destroying a node
  and changing its parent are applied at the end of the frame, so the
  traversal never sees a half-changed tree. Nodes created during a frame start
  on the next one.
- A component has an `enabled` flag **[next]**: a disabled component gets no
  update calls. Disabling after an error uses the same flag.

## Scene systems and the engine context

- **Scene systems** (render list, physics world, audio world) belong to a
  `Scene`; every scene has its own. The Play copy gets its own physics world;
  in the editor the world exists but is not stepped. Systems are attached to
  the scene through one generic mechanism, not as fixed fields, so 2D physics
  can be added next to 3D physics later **[next]**.
- A component finds its system through its scene in `onEnterScene`.
- **The active camera is state of the render system**, not of `Scene`, and it
  resets itself when the camera leaves the scene **[next]**. Today
  `Scene::activeCamera()` is a raw `Camera3D*` reset by hand in several places.
- **Engine services** (input, time, loading and switching scenes) reach the
  scene through an explicit context given by its owner: the application, the
  editor or a test **[next]**. There is no `Application::instance()`. Log and
  translation stay global services by design.
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
