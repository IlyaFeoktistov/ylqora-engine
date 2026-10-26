#pragma once

namespace yqlora {

// Собственные коды клавиш, чтобы игровой код не зависел от SDL.
// Расширяется по мере надобности.
enum class Key {
    Unknown,
    Escape,
    Tab,
    F1,
    F2,
};

} // namespace yqlora
