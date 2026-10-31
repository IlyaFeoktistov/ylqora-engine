# Scripts

[Русская версия](scripting.ru.md)

A script is a component (see [scene](scene.md)): it lives on a node and
is its own component type in every language. This document covers the
languages and hot reload.

Status marks: see [README](README.md).

## Languages

**Scripting languages:** C++ first (MVP), then Luau after the MVP, then
   visual scripts after 2D.

## Boundaries: one behaviour system, several languages

Behaviour is written in C++ now, in Luau and visual graphs later. They share
everything except how the code runs:

```
Editor, inspector, serializer, graph palette, Luau bindings
        |   read only the type registry
Type registry: types, properties (with attributes), methods, phases,
               module, "is a script"
        |   filled by
C++ classes (ClassTypeBuilder)   Luau scripts, visual graphs (TypeBuilder)
        |   instances are
Component: the lifecycle contract (onEnable ... onStop), phase lists,
           enabled, owner node
        |   implemented by
user C++ class : Script          LuauScript (VM)      VisualScript (graph)
```

- **`Component` is the contract.** The lifecycle, the phase lists, enabling
  and exception handling belong to every component; a language backend only
  decides what a call does.
- **The type registry is the one source of metadata.** A property is
  described by values and accessor functions, so a C++ field, a Luau
  variable and a graph input are the same kind of property. The inspector,
  the serializer and the property menu never know which language a field
  comes from.
- **Field attributes live in `Property`** (range, display name, tooltip,
  group, not saved, not shown), so every language gets them at once.
- **"Is a script"** is a mark of the type in the registry, set by every
  backend; "Add component" groups scripts by it. It is not a base class.
- **`Script` is the base of C++ scripts only**: a convenience over the
  engine context the component already has (`node()`, `transform()`,
  `input()`, `time()`, `scene()`). Luau and visual scripts do not derive
  from it; they expose the same things through their bindings, generated
  from the registry.
- **No `ScriptInstance` layer.** Luau and graphs are one C++ component class
  each (`LuauScript`, `VisualScript`) bound to their resource, with their
  type registered at run time; nothing goes through a C++ adapter.
- **Modules own their types**: a C++ game library, `luau`, `visual`. A type
  name taken by another module is refused; unloading a module destroys its
  instances first.
- **Stable identifiers are names, renames are explicit.** Files hold the
  type name (`"Player"`) and property names (`"speed"`), never addresses or
  anything from a library; names keep diffs readable, and a C++ class has no
  file of its own to hold a GUID. A rename is declared where it happens:
  `field("speed", ...).formerly("moveSpeed")`,
  `registerScript<Player>().formerly("Hero")`: the old name is read, the new
  one written, no value is lost. Luau scripts are files, so their types can
  later follow the resource ID of the script.
- **No silent data loss.** A component whose type is missing keeps its JSON
  object exactly as read and writes it back unchanged. Properties a known
  type no longer has are kept with the object and written back too, so
  removing a field and bringing it back loses nothing.

## Scripts in C++ [planned, step 2]

```cpp
class Player : public yqlora::Script {
public:
    YQLORA_SCRIPT(Player)

    float speed = 5.0f;

    // Inside the class, so private fields can be listed too.
    YQLORA_FIELDS(
        field("speed", &Player::speed).range(0.0f, 100.0f).tooltip("player.speed.tooltip"),
        field("jumpHeight", &Player::m_jumpHeight)
    )

protected:
    void onUpdate(float deltaSeconds) override
    {
        const Vec3 move(input().actionValue("MoveX"), 0.0f, -input().actionValue("MoveY"));
        transform().translate(move * speed * deltaSeconds);
    }

private:
    float m_jumpHeight = 1.5f;
};

// The one entry point of the game module, written by hand in step 2 and by
// the generator in step 3.
YQLORA_MODULE_ENTRY(registrar)
{
    registrar.registerScript<Player>();
}
```

### Decisions

- **A script is a component**, not a parallel object system: same node,
  lifecycle, phase lists, serialization and inspector as any component.
- **Fields are listed explicitly**, not taken from the access modifier: a
  field is saved and shown because it is listed, public or private. The name
  in `field("speed", ...)` is the stable identifier in files; the label in
  the inspector is separate (`property.<name>` or `displayName`). A field
  description can grow without breaking scripts: range, display name,
  tooltip, group, and flags for "not saved" and "not shown" (the field list
  is metadata; saving and the inspector are two users of it).
- **Field types** are the property types of the registry: bool, int, float,
  string, vectors, colour, enums, `NodeRef`, `ResourceRef`, lists and
  records. Node and resource references are stable IDs, never pointers.
- **Explicit registration per module**: one entry function of the library
  (`YQLORA_MODULE_ENTRY`) registers its scripts through the
  `ModuleRegistrar` it is given, which records them under the module; no
  global static registrars, no dependence on the order of static
  initialization. A type name already taken (by another module, by the same
  module twice, or as a former name) is not registered: it goes to
  `rejected()` with the module that holds it, so the loader reports it
  instead of crashing the editor. After `registerScript<T>()` only type rules
  chain: `formerly`, `requiresNode`, `requiresComponent`, `executionOrder`;
  fields come from `YQLORA_FIELDS`.
- **The script API** (`node()`, `transform()`, `input()`, `time()`,
  `scene()`, finding nodes, the log) is a convenience over the engine
  context the component already has through its scene; `Script` owns no
  subsystem.

### Lifecycle contract

The component lifecycle of the [scene](scene.md), as scripts see it:

| Call | When |
|---|---|
| `onEnable` | the script starts taking part: enabled and its node active in hierarchy; before `onStart` |
| `onStart` | once per run, before the first phase call; re-enabling does not repeat it |
| `onFixedUpdate(step)` | every fixed step of game time (physics) |
| `onUpdate(delta)` | every frame |
| `onLateUpdate(delta)` | after all `onUpdate` of the frame |
| `onDisable` | at once when the script or its node is switched off, before `onStop`; never twice: removing an already disabled script does not repeat it |
| `onStop` | the game stops or the script is removed or destroyed; only after `onStart` |

- A script disabled before its first start gets no `onStart` until it is
  enabled.
- A script that disables itself in `onUpdate` gets `onDisable` at once and
  is skipped for the rest of the phase; it is dropped from the list after
  the phase.
- Order inside a phase: the execution order of the type, then the order of
  entering (see [scene](scene.md) and Execution Order in the project
  settings).
- An exception from a call disables the script and goes to the log.

### Phases of step 2

1. Field attributes in `Property` (with `formerly`) and the "is a script"
   mark in the registry (for every language); `Script`, its API,
   `YQLORA_SCRIPT`, `YQLORA_FIELDS`, `registerScript` **[done]**.
2. Module registration: the entry function, `registerScript` (with
   `formerly`), refusal of a type name taken by another module **[done]**.
3. `GameModule`: load a shared library, register its types, unload. Before
   unloading, every instance of its types is destroyed (scenes of the editor
   and of Play), then `removeModule`, then the library goes. A test module
   built by the repository goes through the whole cycle on Linux and
   Windows: load, register, create, lifecycle calls, destroy, unload
   **[done]**. The library is found by `kModuleEntrySymbol`, the name
   `YQLORA_MODULE_ENTRY` defines. `GameModule::unload` destroys the
   components in the scenes it is given; instances outside scenes (the edit
   history) are the owner's to destroy first. Types are looked up by name,
   never by `typeid`: `type_info` comparison across libraries is unreliable
   on MinGW, so `TypeRegistry::find<T>()` and `Scene::system<T>()` use the
   `YQLORA_TYPE` name.
4. Editor: scripts in "Add component" as their own group (by the mark),
   fields in the inspector with their attributes, saving and loading of field values; a component whose type is
   missing (module not built, script deleted) is kept as a placeholder with
   its JSON object as read, shown as missing and written back unchanged,
   and comes back when the type returns; properties a known type no longer
   has are kept and written back. An example movement
   script on the default input actions.
5. Documents: this one, the game module, a "first script" guide and the
   limits (native code can crash the editor; one game module per project;
   how Luau and graphs fit the same boundaries).

Step 3 adds the project build, `YQLORA_FIELD()` with a generator built on
Clang LibTooling (a full C++ parser, never regular expressions; it writes
code the compiler checks), and hot reload.

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
3. Destroy the tree, `TypeRegistry::removeModule`, unload the library
   (`GameModule::unload`).
4. Load the new library, register its types, load the scene back.

A component whose type is missing (the game failed to compile, a script was
deleted) is kept as a placeholder holding its raw data, shown in the inspector
as missing, and written back unchanged on save. Otherwise saving the scene
would silently drop the data **[planned, step 2]**.
