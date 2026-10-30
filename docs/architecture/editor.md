# Editor

[Русская версия](editor.ru.md)

The editor is an application on the engine: the project manager, scene
editing and Play. Its logic lives in `editor/src` without ImGui, drawing in
`editor/ui`.

Status marks: see [README](README.md).

## Parts

- `EditorApp` assembles the editor: theme, settings, actions, panels.
- `EditorSession` holds the open project, the scene document, Play and the
  selection.
- `SceneDocument` is the edited scene, its file and its edit history.
- `PlayMode` is the Play copy of the scene with its own services.
- Panels in `editor/ui` get an `EditorContext` every frame.

## Splitting [next]

- Editing (creating and deleting nodes, components, properties, transforms,
  undo and redo) moves from `EditorSession` to `SceneEditing`, which changes
  the current scene through commands.
- The main scene of the project moves to the project settings, section
  General (see [project settings](project-settings.md)).
- Opening and closing a project (services, notifying panels, the list of
  projects) moves from `EditorApp` to the session; the window title is built
  by a function without ImGui.
- `EditorContext` gives panels an `EditorHost` instead of the whole
  `Application`: the renderer, the folder dialog, the language, quitting,
  mouse wrapping.
- The name of a resource in the inspector (a built-in mesh, a path in the
  project) is resolved without ImGui, in `editor/src`.
