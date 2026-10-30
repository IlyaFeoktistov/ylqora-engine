# Scripts

[Русская версия](scripting.ru.md)

A script is a component (see [scene](scene.md)): it lives on a node and
is its own component type in every language. This document covers the
languages and hot reload.

Status marks: see [README](README.md).

## Languages

**Scripting languages:** C++ first (MVP), then Luau after the MVP, then
   visual scripts after 2D.

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

- Every `.luau` script registers **its own component type**, named after the
  script, with the properties declared in the script. All of them create
  instances of one C++ class, `LuauScript`, bound to the script resource. The
  registry takes such types without a C++ class of their own, see
  [types and reflection](reflection.md).
- Bindings are generated from the registry: properties and methods of the
  types. One list serves Luau, the inspector and the API documentation. The
  same list produces type definition files for luau-lsp, which gives
  completion and type checking in an editor.
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
