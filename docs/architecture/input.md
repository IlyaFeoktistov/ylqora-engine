# Input

[Русская версия](input.ru.md)

Keyboard, mouse and gamepads, and the input actions that games read instead
of keys.

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
  - an **axis** is the sum of the values of the bindings times their scales,
    clamped to -1..1: `D` with 1 and `A` with -1 make a horizontal axis, a
    stick adds its own deflection. Mouse movement is a delta, not a
    deflection, so an axis with a mouse binding is not clamped and the scale
    is the sensitivity.
- A binding names a control: a key (`"Space"`), a mouse button
  (`"MouseLeft"`), a mouse axis (`"MouseX"`, `"MouseY"`, `"MouseWheel"`), a
  gamepad button (`"GamepadSouth"`) or a gamepad axis (`"GamepadLeftStickX"`,
  `"GamepadRightTrigger"`). Names, not numbers, are written to the file, so
  the order in code can change. An unknown name is ignored.
- Games ask for actions, not keys: `input.isActionDown("Jump")`,
  `wasActionPressed`, `wasActionReleased`, `actionValue("MoveX")`. An
  unknown action is not down and gives 0. Rebinding never touches game code.
- The actions are computed in the input stage of the frame, right after the
  window events, from the settings of the open project
  (`InputFeeder::updateActions`); an edit of the settings applies from the
  next frame.
- In the settings window a binding is set by pressing: the button of the
  binding waits for the next key, mouse or gamepad button or a stick pushed
  past half way; Escape cancels. A list next to it holds every control,
  mouse axes included, which cannot be set by pressing.

## Gamepads [done]

- `Gamepads` (platform) opens gamepads through SDL as they connect. There is
  one player: the first connected gamepad is theirs; when it is
  disconnected its buttons are released and the next connected one takes
  its place. `Gamepads` belongs to the application, not to the window, so a
  window rebuilt for another graphics API keeps them.
- Buttons are named by position, not by letter: `South` is the bottom button
  (A on Xbox, cross on PlayStation).
- Sticks give -1..1 with up and right positive (like `W` and `D`), triggers
  0..1. A dead zone (0.2 for sticks, 0.1 for triggers) gives exactly 0 at
  rest; past it the value grows from 0 without a jump.
