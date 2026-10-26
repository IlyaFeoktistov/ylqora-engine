#pragma once

namespace yqlora {

// Собственные коды клавиш, чтобы игровой код не зависел от SDL.
// Клавиши физические: W остаётся W в любой раскладке.
enum class Key {
    Unknown,

    Escape,
    Tab,
    Space,
    Enter,
    Backspace,
    Delete,

    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,

    Up,
    Down,
    Left,
    Right,

    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,

    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    Count, // не клавиша, а число элементов
};

enum class MouseButton {
    Left,
    Right,
    Middle,

    Count, // не кнопка, а число элементов
};

} // namespace yqlora
