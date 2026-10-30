#include "yqlora/input/InputControl.hpp"

#include "yqlora/input/Input.hpp"

#include <algorithm>
#include <cmath>

namespace yqlora {

namespace {

// Ось геймпада, отклонённая дальше этого, задаёт привязку нажатием.
constexpr float kActivationThreshold = 0.5f;

template <typename Enum, std::size_t Count>
std::optional<Enum> findName(const std::array<std::string_view, Count>& names, std::string_view name)
{
    const auto it = std::ranges::find(names, name);

    return it != names.end() ? std::optional(static_cast<Enum>(it - names.begin())) : std::nullopt;
}

template <typename Enum, std::size_t Count>
std::string_view nameOf(const std::array<std::string_view, Count>& names, Enum value)
{
    return names[static_cast<std::size_t>(value)];
}

float mouseAxisValue(const Input& input, MouseAxis axis)
{
    switch (axis) {
    case MouseAxis::X: return input.mouseDelta().x;
    // Окно считает вниз, игре удобнее вверх: как у стика.
    case MouseAxis::Y: return -input.mouseDelta().y;
    case MouseAxis::Wheel: return input.wheelDelta();
    case MouseAxis::Count: break;
    }

    return 0.0f;
}

} // namespace

std::vector<InputControl> allInputControls()
{
    std::vector<InputControl> result;
    const auto append = [&]<typename Enum>(std::size_t count, Enum first) {
        for (std::size_t i = static_cast<std::size_t>(first); i < count; ++i) {
            result.emplace_back(static_cast<Enum>(i));
        }
    };
    append(kKeyNames.size(), Key::Escape);
    append(kMouseButtonNames.size(), MouseButton::Left);
    append(kMouseAxisNames.size(), MouseAxis::X);
    append(kGamepadButtonNames.size(), GamepadButton::South);
    append(kGamepadAxisNames.size(), GamepadAxis::LeftStickX);

    return result;
}

std::optional<InputControl> controlFromName(std::string_view name)
{
    if (const auto key = findName<Key>(kKeyNames, name); key && *key != Key::Unknown) {
        return *key;
    }
    if (const auto button = findName<MouseButton>(kMouseButtonNames, name)) {
        return *button;
    }
    if (const auto axis = findName<MouseAxis>(kMouseAxisNames, name)) {
        return *axis;
    }
    if (const auto button = findName<GamepadButton>(kGamepadButtonNames, name)) {
        return *button;
    }
    if (const auto axis = findName<GamepadAxis>(kGamepadAxisNames, name)) {
        return *axis;
    }

    return std::nullopt;
}

std::string_view controlName(const InputControl& control)
{
    return std::visit(
        [](auto value) -> std::string_view {
            using T = decltype(value);
            if constexpr (std::is_same_v<T, Key>) {
                return nameOf(kKeyNames, value);
            } else if constexpr (std::is_same_v<T, MouseButton>) {
                return nameOf(kMouseButtonNames, value);
            } else if constexpr (std::is_same_v<T, MouseAxis>) {
                return nameOf(kMouseAxisNames, value);
            } else if constexpr (std::is_same_v<T, GamepadButton>) {
                return nameOf(kGamepadButtonNames, value);
            } else {
                return nameOf(kGamepadAxisNames, value);
            }
        },
        control
    );
}

float controlValue(const Input& input, const InputControl& control)
{
    return std::visit(
        [&](auto value) -> float {
            using T = decltype(value);
            if constexpr (std::is_same_v<T, Key>) {
                return input.isKeyDown(value) ? 1.0f : 0.0f;
            } else if constexpr (std::is_same_v<T, MouseButton>) {
                return input.isMouseButtonDown(value) ? 1.0f : 0.0f;
            } else if constexpr (std::is_same_v<T, MouseAxis>) {
                return mouseAxisValue(input, value);
            } else if constexpr (std::is_same_v<T, GamepadButton>) {
                return input.isGamepadButtonDown(value) ? 1.0f : 0.0f;
            } else {
                return input.gamepadAxis(value);
            }
        },
        control
    );
}

bool isUnbounded(const InputControl& control)
{
    return std::holds_alternative<MouseAxis>(control);
}

bool wasControlActivated(const Input& input, const InputControl& control)
{
    return std::visit(
        [&](auto value) -> bool {
            using T = decltype(value);
            if constexpr (std::is_same_v<T, Key>) {
                return input.wasKeyPressed(value);
            } else if constexpr (std::is_same_v<T, MouseButton>) {
                return input.wasMouseButtonPressed(value);
            } else if constexpr (std::is_same_v<T, MouseAxis>) {
                return false;
            } else if constexpr (std::is_same_v<T, GamepadButton>) {
                return input.wasGamepadButtonPressed(value);
            } else {
                return std::abs(input.gamepadAxis(value)) > kActivationThreshold;
            }
        },
        control
    );
}

} // namespace yqlora
