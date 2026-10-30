# Code editor [planned, after MVP]

[Русская версия](code-editor.ru.md)

Scripts are written in the editor itself: a "Scripts" tab in the bottom
panel, next to "Files". It comes with Luau; later the same tab edits shaders.

Status marks: see [README](README.md).

## The tab

- The bottom panel has the tabs "Files" and "Scripts", later "Shaders". A
  double click on a `.luau` file in "Files" opens it in "Scripts".
- Every open file is a tab of its own, a modified one is marked with `*`.
  Ctrl+S saves it; saving a script reloads the Luau module (see
  [scripts](scripting.md)).
- Syntax highlighting, completion, hints on hover, errors underlined in the
  text and listed under it, go to definition.
- An external editor (VS Code and the like) can be chosen in the editor
  settings instead; the tab then only opens the file there.

## Where the logic lives

As the rest of the editor, the logic is apart from ImGui:

- `TextDocument`: the text of one file, cursor and selection, edit history,
  loading, saving, the modified flag.
- A language: file extensions, a highlighter that splits a line into tokens
  (keyword, string, number, comment, name), and a language server if there
  is one. Colours of the tokens come from the editor theme.
- `LanguageClient`: talks to a language server over the Language Server
  Protocol (stdio): completion, hover, diagnostics, definitions.
- The widget in `editor/ui` only draws a `TextDocument` and passes keys and
  the mouse to it.

## Hints for Luau

- The language server is [luau-lsp](https://github.com/JohnnyMorganz/luau-lsp)
  (MIT), one process per open project. Without it highlighting still works,
  only hints are missing.
- It learns the engine types from a definition file generated from the type
  registry (properties and methods, see [reflection](reflection.md)). The
  file is written into the project when it is opened and after every reload
  of a module, so the game's own types are in the hints too.

## Shaders later

- The same `TextDocument` and tab, a GLSL highlighter, errors from the shader
  compiler of the engine; saving reloads the shader (see
  [rendering](rendering.md)).

## Open question

- The text widget: an existing one (ImGuiColorTextEdit, MIT) or our own on
  top of `TextDocument`. Decided when the work starts.
