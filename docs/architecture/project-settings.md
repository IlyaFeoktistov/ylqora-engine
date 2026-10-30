# Project settings

[Русская версия](project-settings.ru.md)

Settings of a game that live in its project folder, are shared by everyone who
works on it and go to version control: input actions, time step, graphics API,
physics, build targets. In the editor: **Project → Project Settings**.

Status marks as in [README](README.md): **[done]**, **[next]** (before C++
scripts), **[planned, step N]**.

Not to be confused with **editor preferences** (theme, language, camera
speed): those belong to the user, live in the user profile (`Settings`) and
never go into the project.

## Design decisions

1. **A settings section is a class with registered properties**, in the same
   type registry as nodes and components (`TypeKind::Settings`). One
   description gives the file format, the editor window (the same property
   editor as the inspector) and access from code. No second list of fields
   anywhere.
2. **Each section belongs to its system.** Physics declares its section,
   input declares its own; there is no class with all settings. A game module
   may register sections of its own; they show up in the window like the
   engine ones and leave with the module on reload.
3. **One file per section**: `settings/<section>.json` in the project folder.
   Fewer merge conflicts, a readable diff. The files are part of the project
   format version: a change of their format means a migration and the project
   upgrade window, as for scenes.
4. **Edits are undone with Ctrl+Z**, through the same edit history as scene
   edits. The history therefore holds commands on the project, not only on
   the scene.
5. **Input goes through actions from the start.** Scripts ask for
   `input.action("Jump")`, not for a key, so rebinding never touches game code.

## Sections

| Section | Contents | When |
|---|---|---|
| General | game name, version, company, icon, startup scene | **[next]** |
| Time | fixed time step, longest frame (today the constant `kMaxFrameSeconds`) | **[next]**, with the fixed step |
| Input | actions and axes, their bindings to keys, mouse buttons, gamepad | **[next]** |
| Execution Order | order of component types inside every lifecycle phase, overriding the default from code (see [scene](scene.md)) | **[next]** |
| Layers | names of the 32 node layers used by camera masks and physics | **[next]** |
| Graphics | graphics API per platform (OpenGL; Vulkan and D3D12 come with the RHI), vsync | **[next]** |
| Scripts | C++ build: compiler, configuration; Luau later | **[planned, step 3]** |
| Physics | gravity, layers, collision matrix | **[planned, step 6]** |
| Audio | master volume, sample rate | **[planned, step 7]** |
| Display | resolution, fullscreen, resizable window | **[planned, step 9]** |
| Build | target platforms (Windows, Linux), scenes in the game, output folder, debug or release | **[planned, step 9]** |

The startup scene moves from `project.yqlora` into General.

## Registration

```cpp
struct TimeSettings {
    YQLORA_SETTINGS(TimeSettings, "time")   // type name and file name

    float fixedStepSeconds = 1.0f / 60.0f;
    float maxFrameSeconds = 0.25f;
};

registry.registerSettings<TimeSettings>()
    .property("fixedStep", &TimeSettings::fixedStepSeconds)
    .property("maxFrame", &TimeSettings::maxFrameSeconds);
```

The exact macro and builder follow the node and component ones; what matters
is that the file name, the type name and the fields are written once.

## Access

- The project owns one instance of every registered section, created with
  defaults and filled from its file. A missing file means defaults; a file of
  a section nobody registers any more is kept as is and written back
  unchanged, so a module that failed to load does not lose its settings.
- A system reads its section from the project it runs with:
  `project.settings<PhysicsSettings>()`. The runtime (step 9) loads the same
  files.
- Settings that cannot change while running (the graphics API) carry a
  "restart required" hint; the window says so next to the value. All others
  apply at once, including during Play.

## Input actions

- An action has a name and a kind: a button (pressed, held, released) or an
  axis (a value from -1 to 1, from two keys or a stick).
- A binding is a key, a mouse button or a gamepad button or axis. One action
  has any number of bindings.
- `Input` keeps the raw state (keys, mouse) as today; actions are a layer over
  it, read by scripts and the HUD.

## Layers

As Tags and Layers in Unity:

- 32 layers. Layer 0 is `Default`, always there and not renamed; the others
  are named by the project. A layer without a name is unused and does not
  appear in the lists.
- A node and a camera mask store layer numbers, so renaming a layer changes
  nothing in scenes. Clearing the name of a layer still in use asks first and
  leaves the nodes on it.
- The layer list in the inspector and the culling mask show the names and end
  with "Edit layers...", which opens this section.
- Physics (step 6) takes the same layers for its collision matrix.

## What the registry needs

- List properties (actions, bindings, layers): today a property holds one
  value only.
- A binding property type (key, mouse button, gamepad input) with its editor
  widget: press to capture.
- The settings kind (`TypeKind::Settings`) and its builder.
