# Examples

[Русская версия](README.ru.md)

`scripts/` is a game module with example scripts, built with the repository
(`YQLORA_BUILD_EXAMPLES`, on by default):

- `Rotator` turns its node around an axis; its fields (speed with a range,
  axis) show up in the inspector.

The library is named like the game library of a project (`game.dll`,
`libgame.so`) and lands in `build/<preset>/examples/scripts/`. To try it,
copy it into the `.build/` folder of a project and open the project in the
editor: the scripts appear under "Scripts" in "Add component". In step 3 the
editor builds the scripts of a project itself.
