# Editor

[Русская версия](editor.ru.md)

The editor is an application on the engine: the project manager, scene
editing and Play. Its logic lives in `editor/src` without ImGui, drawing in
`editor/ui`.

Status marks: see [README](README.md).

## Parts

- `EditorApp` assembles the editor: theme, settings, actions, panels.
- `EditorSession` holds the open project, the scene document, Play, the
  selection, the edit history (undo and redo of the scene and of the project
  settings), `SceneEditing` and `SettingsEditing`.
- `SceneDocument` is the edited scene and its file.
- **Unsaved changes** follow one rule for the scene and the project
  settings: a thing is unsaved while it differs from its file, however it
  was changed, through the history or not. The scene document keeps the
  scene as written to its file and compares after every edit, undo and redo,
  twice a second otherwise, and right before asking about unsaved changes;
  the settings compare their sections the same way. The one question is
  `EditorSession::modified()`; the history is only undo and redo **[done]**.
- `PlayMode` is the Play copy of the scene with its own services.
- Panels in `editor/ui` get an `EditorContext` every frame.

## Splitting [done]

- Editing (creating and deleting nodes, components, properties, transforms)
  lives in `SceneEditing` (`session.editing()`), which changes the current
  scene through commands. Project settings are changed by `SettingsEditing`
  (`session.settingsEditing()`); undo and redo are the session's, the history
  is shared.
- The session puts the open project into the engine services itself when it
  opens and closes a project; the window title is built by `windowTitle`,
  without ImGui.
- `EditorContext` gives panels an `EditorHost` instead of the whole
  `Application`: the renderer, the depth range, input, the language, the
  folder dialog. `RenderView` takes only the renderer.
- The scene document owns its scene; the application has none.
- The name of a resource in the inspector (a built-in mesh, a path in the
  project) is resolved by `resourceLabel` in `editor/src`, without ImGui.
- The main scene of the project lives in the project settings, section
  General (see [project settings](project-settings.md)) **[done]**.

## Panels and property fields [done]

- Every panel opens its window through `EditorPanel::beginWindow`: one place
  for what all windows share.
- **Maximize**: any panel can fill the whole editor window and go back. A
  square button left of the close button (in the title of a floating window,
  in the tab bar of a docked one), Shift+Space for the focused panel, a
  right click on the tab or title, a double click on a docked tab, or View ->
  Maximize window. The maximized panel is a separate ImGui window over the
  work area; the dock layout is kept alive but not shown, so it comes back
  unchanged.
- **Property menu**: a right click on a property row in the inspector or the
  settings window. Every menu item says what it acts on:
  - "Reset value to default", "Copy value", "Paste value" act on the whole
    value: three numbers of a position, a colour, a list. The default is the
    value of a new object of the same type (`defaultValue`; a built-in
    component takes it from a new node of the owner's type). Copy puts the
    value on the system clipboard as the same JSON as in the files, so a
    position can be pasted into a scale; a string property takes any text.
  - A right click on one number of a vector (its X, Y, Z or W label or field)
    adds, above the whole value items, "Reset X", "Copy X", "Paste into X"
    for that number alone. "Paste into X" takes only a single number; three
    numbers are pasted with "Paste value".
  - Ctrl+C and Ctrl+V inside a field being edited copy its text, as in any
    text field.
  - Reset and paste are ordinary edits and are undone with Ctrl+Z.
- The label column of a property table fits the longest label, up to half
  the width; a clipped label shows its full text as a tooltip.
