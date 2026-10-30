#pragma once

#include <array>
#include <cstddef>
#include <string_view>

// Список клавиш и кнопок мыши задан один раз: ENTRY - макрос, который
// подставляет тот, кто разворачивает список (значение перечисления, строка
// с именем). Так перечисление и имена не могут разойтись. По именам привязки
// действий ввода хранятся в файлах настроек, поэтому порядок здесь можно менять.
#define YQLORA_KEYS(ENTRY)                                                                                             \
    ENTRY(Unknown)                                                                                                     \
    ENTRY(Escape)                                                                                                      \
    ENTRY(Tab)                                                                                                         \
    ENTRY(Space)                                                                                                       \
    ENTRY(Enter)                                                                                                       \
    ENTRY(Backspace)                                                                                                   \
    ENTRY(Delete)                                                                                                      \
    ENTRY(LeftShift)                                                                                                   \
    ENTRY(RightShift)                                                                                                  \
    ENTRY(LeftCtrl)                                                                                                    \
    ENTRY(RightCtrl)                                                                                                   \
    ENTRY(LeftAlt)                                                                                                     \
    ENTRY(RightAlt)                                                                                                    \
    ENTRY(Up)                                                                                                          \
    ENTRY(Down)                                                                                                        \
    ENTRY(Left)                                                                                                        \
    ENTRY(Right)                                                                                                       \
    ENTRY(A)                                                                                                           \
    ENTRY(B)                                                                                                           \
    ENTRY(C)                                                                                                           \
    ENTRY(D)                                                                                                           \
    ENTRY(E)                                                                                                           \
    ENTRY(F)                                                                                                           \
    ENTRY(G)                                                                                                           \
    ENTRY(H)                                                                                                           \
    ENTRY(I)                                                                                                           \
    ENTRY(J)                                                                                                           \
    ENTRY(K)                                                                                                           \
    ENTRY(L)                                                                                                           \
    ENTRY(M)                                                                                                           \
    ENTRY(N)                                                                                                           \
    ENTRY(O)                                                                                                           \
    ENTRY(P)                                                                                                           \
    ENTRY(Q)                                                                                                           \
    ENTRY(R)                                                                                                           \
    ENTRY(S)                                                                                                           \
    ENTRY(T)                                                                                                           \
    ENTRY(U)                                                                                                           \
    ENTRY(V)                                                                                                           \
    ENTRY(W)                                                                                                           \
    ENTRY(X)                                                                                                           \
    ENTRY(Y)                                                                                                           \
    ENTRY(Z)                                                                                                           \
    ENTRY(Num0)                                                                                                        \
    ENTRY(Num1)                                                                                                        \
    ENTRY(Num2)                                                                                                        \
    ENTRY(Num3)                                                                                                        \
    ENTRY(Num4)                                                                                                        \
    ENTRY(Num5)                                                                                                        \
    ENTRY(Num6)                                                                                                        \
    ENTRY(Num7)                                                                                                        \
    ENTRY(Num8)                                                                                                        \
    ENTRY(Num9)                                                                                                        \
    ENTRY(F1)                                                                                                          \
    ENTRY(F2)                                                                                                          \
    ENTRY(F3)                                                                                                          \
    ENTRY(F4)                                                                                                          \
    ENTRY(F5)                                                                                                          \
    ENTRY(F6)                                                                                                          \
    ENTRY(F7)                                                                                                          \
    ENTRY(F8)                                                                                                          \
    ENTRY(F9)                                                                                                          \
    ENTRY(F10)                                                                                                         \
    ENTRY(F11)                                                                                                         \
    ENTRY(F12)

#define YQLORA_MOUSE_BUTTONS(ENTRY)                                                                                    \
    ENTRY(Left)                                                                                                        \
    ENTRY(Right)                                                                                                       \
    ENTRY(Middle)

namespace yqlora {

// Собственные коды клавиш, чтобы игровой код не зависел от SDL.
// Клавиши физические: W остаётся W в любой раскладке.
enum class Key {
#define YQLORA_ENUM_VALUE(name) name,
    YQLORA_KEYS(YQLORA_ENUM_VALUE) Count, // не клавиша, а число элементов
};

enum class MouseButton {
    YQLORA_MOUSE_BUTTONS(YQLORA_ENUM_VALUE)
#undef YQLORA_ENUM_VALUE
    Count, // не кнопка, а число элементов
};

// Имена по порядку перечислений: "Space", "A"; у кнопок мыши - с приставкой: "MouseLeft".
inline constexpr std::array<std::string_view, static_cast<std::size_t>(Key::Count)> kKeyNames{
#define YQLORA_KEY_NAME(name) #name,
    YQLORA_KEYS(YQLORA_KEY_NAME)
#undef YQLORA_KEY_NAME
};

inline constexpr std::array<std::string_view, static_cast<std::size_t>(MouseButton::Count)> kMouseButtonNames{
#define YQLORA_MOUSE_BUTTON_NAME(name) "Mouse" #name,
    YQLORA_MOUSE_BUTTONS(YQLORA_MOUSE_BUTTON_NAME)
#undef YQLORA_MOUSE_BUTTON_NAME
};

} // namespace yqlora
