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
