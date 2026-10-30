# Input

[Русская версия](input.ru.md)

Keyboard, mouse and, next, gamepads, and the input actions that games read
instead of keys.

Status marks: see [README](README.md).

## Raw input [done]

- `Input` is the state of one frame: keys and mouse buttons down, pressed and
  released in this frame, the mouse position, its movement and the wheel.
  Games only read it.
- Only `InputFeeder` writes it: the window passes the events of the system
  there. A frame begins with `beginFrame` (what lasted one frame is cleared);
  when the window loses focus everything is released, so a key let go in
  another window does not stay down.
- Keys are physical: `W` stays `W` in any keyboard layout. The list of keys
  and mouse buttons is written once (`YQLORA_KEYS`, `YQLORA_MOUSE_BUTTONS` in
  `Key.hpp`); the enums and the names (`kKeyNames`, `kMouseButtonNames`) are
  made from it.
- Mouse wrapping: while the editor camera flies, a cursor that reaches the
  edge of the window moves to the opposite edge and the movement has no jump
  (`Window::setMouseWrapping`).
- In the editor the Play copy has its own `Input`; it sees the input of the
  application only while the Game window is focused (see [frame](frame.md)).

## Input actions [done]

- The Input section of the project settings (`InputSettings`) lists the
  actions. An action has a name, a kind and bindings:
  - a **button** is down while any binding is down; pressed and released
    come from the change of that against the previous frame, so a second key
    holding the action does not make it released;
  - an **axis** is the sum of the scales of the bindings that are down,
    clamped to -1..1: `D` with 1 and `A` with -1 make a horizontal axis.
- A binding names a control: a key (`"Space"`) or a mouse button
  (`"MouseLeft"`). Names, not numbers, are written to the file, so the order
  of keys in code can change. An unknown name is ignored.
- Games ask for actions, not keys: `input.isActionDown("Jump")`,
  `wasActionPressed`, `wasActionReleased`, `actionValue("MoveX")`. An
  unknown action is not down and gives 0. Rebinding never touches game code.
- The actions are computed in the input stage of the frame, right after the
  window events, from the settings of the open project
  (`InputFeeder::updateActions`); an edit of the settings applies from the
  next frame.
- In the settings window a binding is set by pressing: the button of the
  binding waits for the next key or mouse button, Escape cancels.

## Gamepads [next]

- Gamepad buttons and sticks through SDL, with a dead zone; they become
  controls of bindings like keys (`"GamepadSouth"`, `"LeftStickX"`), sticks
  give axis values.
