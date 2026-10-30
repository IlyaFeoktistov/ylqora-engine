# Architecture

[Русская версия](README.ru.md)

Contracts of the engine subsystems, one document per subsystem. Each changes
together with the code it describes.

Status marks: **[done]** implemented, **[next]** the next stage of work
(before C++ scripts), **[planned]** later, with the step it belongs to.

| Document | What it covers |
|---|---|
| [Scene](scene.md) | nodes, components, resources, lifecycle, references and serialization |
| [Types and reflection](reflection.md) | type registry, properties, methods, types declared at run time |
| [Engine context and the frame](frame.md) | engine services, game time, scene systems, the order of a frame |
| [Rendering](rendering.md) | cameras, layers, views, editor overlay |
| [Scripts](scripting.md) | C++, Luau, visual scripts, hot reload |
| [Editor](editor.md) | parts of the editor: session, document, Play, panels |
| [Code editor](code-editor.md) | the Scripts tab: Luau with highlighting and hints, later shaders |
| [Plugins](plugins.md) | ready-made scripts, scenes and resources from the plugin repository, the plugin manager |
| [Project settings](project-settings.md) | settings of a game kept in its project |

## Code layout

Each subsystem is a folder of its own and registers its own types
(`registerSceneTypes`, `registerRenderTypes` in `internal/`);
`registerEngineTypes` in `core/` only calls them in order. Public headers are
in `include/`, headers for the engine and the editor only in `internal/`,
`src/` holds only `.cpp` files.

```
engine/include/yqlora/core/        engine services, engine types, log, version
engine/include/yqlora/reflection/  TypeRegistry, Property, Method, component rules
engine/include/yqlora/scene/       Node, Node3D, Transform3D, Component, Scene, scene systems
engine/include/yqlora/render/      Camera3D, MeshRenderer, RenderSystem; lights        [step 5]
engine/include/yqlora/physics/     RigidBody, colliders, PhysicsWorld                  [step 6]
engine/include/yqlora/audio/       AudioSource, AudioListener, AudioWorld              [step 7]
engine/include/yqlora/ui/          Control, Label, Button                              [step 8]
```

Platform and interface **[done]**: the interface layer (`UiLayer`, Dear
ImGui) lives in `ui/` and gets window events through `WindowEvents::onEvent`,
so the window knows nothing about ImGui; system dialogs are the platform
`FileDialogs`, not part of `Window`; upgrading a project to the current format
is the free function `upgradeProject` of `project/`.
