# Localization

[Русская версия](localization.ru.md)

Texts of the editor and, later, of games in several languages.

Status marks: see [README](README.md).

## Files and keys [done]

- One file per language: `assets/locales/<code>.json` (`en.json`,
  `ru.json`). Nested objects become dotted keys:
  `{"editor": {"menu": {"save": "Save scene"}}}` gives `editor.menu.save`.
- `language.name` is the name of the language in itself ("Русский"); the
  language menu is built from the files found, nothing else lists languages.
- Key groups:

| Keys | What |
|---|---|
| `editor.*` | texts of the editor: menus, windows, panels, dialogs |
| `projects.*` | the project manager |
| `type.<Type>.description` | tooltip of a registered type |
| `property.<name>` | label of a property in the inspector and the settings window |
| `editor.settings.<Section>` | name of a settings section in the settings window |

- Names of nodes, types and properties in scene files are not translated:
  they are identifiers, always in English.

## Lookup [done]

- `tr(key)` gives the text in the current language. A key missing in it is
  taken from English; a key missing everywhere comes back as the key itself,
  so a forgotten translation is seen, not an empty string.
- A property without a `property.<name>` key is shown by its name with a
  capital letter, so a game type works before anyone translates it.
- The language is chosen by the user (View -> Language), at first the first
  system language that has a file; the choice is kept in the user settings,
  not in a project.
- `tr` is one of the two services of the engine with a global access point
  (the other is the log): the application sets the active `Localization`.

## Rules [done]

- Every registered engine type has a description in every language; a test
  checks it, so a new type without `type.<Type>.description` in `en.json` and
  `ru.json` fails the tests.
- Texts are added to all language files in the same change.

## Planned

- Translations for games (texts of the HUD, a language setting of the game)
  come with the player interface **[planned, step 8]**.
