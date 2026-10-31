# Examples

[Русская версия](README.ru.md)

`scripts/` is a game module with example scripts, built with the repository
(`YQLORA_BUILD_EXAMPLES`, on by default):

- `Rotator` turns its node around an axis; its fields (speed with a range,
  axis) show up in the inspector.

It is built with `yqlora_add_game_module`, the same rules a project gets
through the SDK, and the tests load it. To use an example in a project,
copy its file anywhere into the project: the editor builds every `.cpp`
there, and the script registers itself (`YQLORA_REGISTER_SCRIPT`; see
[first script](../docs/guide/first-script.md)).
