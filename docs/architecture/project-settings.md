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
   the scene: it belongs to `EditorSession`, not to the scene document. A
   settings edit is written to its file at once and does not make the scene
   unsaved; opening another scene clears the history, settings edits
   included (as in Godot) **[done]**.
5. **Input goes through actions from the start.** Scripts ask for
   `input.action("Jump")`, not for a key, so rebinding never touches game code.

## Sections

| Section | Contents | When |
|---|---|---|
| General | game name, version, company, startup scene **[done]**; icon | **[planned, step 4]** for the icon, with textures |
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

The startup scene lives in General (`GeneralSettings::startupScene`);
`project.yqlora` keeps only the project name and the engine version **[done]**.

## Registration

```cpp
class TimeSettings final : public SettingsSection {
public:
    YQLORA_TYPE(TimeSettings, SettingsSection)   // type name, as for nodes

    float fixedStepSeconds = 1.0f / 60.0f;
    float maxFrameSeconds = 0.25f;
};

registry.registerSettings<TimeSettings>("time")  // settings/time.json
    .property("fixedStep", &TimeSettings::fixedStepSeconds)
    .property("maxFrame", &TimeSettings::maxFrameSeconds);
```

`SettingsSection` is the root of sections, like `Node` and `Component`. The
file name, the type name and the fields are written once **[done]**. A file
holds the format version and the values under `values`, so a property named
`version` does not clash with the format key.

## Access

- The project owns one instance of every registered section
  (`ProjectSettings`), created with defaults and filled from its file. A
  missing file means defaults; a file of a section nobody registers any more
  is never touched, so a module that failed to load does not lose its
  settings **[done]**.
- A system reads its section from the project it runs with:
  `project.settings().get<PhysicsSettings>()` **[done]**. The runtime
  (step 9) loads the same files.
- Sections of a game module are recreated when the module is reloaded, so
  the project holds no objects from an unloaded dll **[planned, step 3]**.
- Settings that cannot change while running (the graphics API) carry a
  "restart required" hint; the window says so next to the value. All others
  apply at once, including during Play.

## Window

**Project -> Project Settings**: sections on the left, the properties of the
selected one on the right, drawn by the same property table as the inspector.
The settings folder and the project file are hidden from the Files panel and
cannot be changed there. Above the sections the window shows the project
version (the format of the project files) as plain text: it is not a setting
and changes only through the project upgrade, never by hand. The version of
the game itself, for players, is the editable `gameVersion` in General
**[done]**.

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
- The settings kind (`TypeKind::Settings`) and its builder **[done]**.
