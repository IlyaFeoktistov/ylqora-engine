#include "yqlora/input/InputSettings.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/project/Project.hpp"

#include <utility>

namespace yqlora {

namespace {

// Сдвиг мыши в точках против отклонения стика от -1 до 1: без множителя
// мышь поворачивала бы обзор во много раз быстрее стика.
constexpr float kMouseLookScale = 0.1f;

InputAction button(std::string name, std::vector<InputBinding> bindings)
{
    return {.name = std::move(name), .kind = InputActionKind::Button, .bindings = std::move(bindings)};
}

InputAction axis(std::string name, std::vector<InputBinding> bindings)
{
    return {.name = std::move(name), .kind = InputActionKind::Axis, .bindings = std::move(bindings)};
}

} // namespace

std::vector<InputAction> defaultInputActions()
{
    return {
        axis("MoveX", {{"D", 1.0f}, {"A", -1.0f}, {"Right", 1.0f}, {"Left", -1.0f}, {"GamepadLeftStickX", 1.0f}}),
        axis("MoveY", {{"W", 1.0f}, {"S", -1.0f}, {"Up", 1.0f}, {"Down", -1.0f}, {"GamepadLeftStickY", 1.0f}}),
        axis("LookX", {{"MouseX", kMouseLookScale}, {"GamepadRightStickX", 1.0f}}),
        axis("LookY", {{"MouseY", kMouseLookScale}, {"GamepadRightStickY", 1.0f}}),
        button("Jump", {{"Space"}, {"GamepadSouth"}}),
        button("Sprint", {{"LeftShift"}, {"GamepadLeftStick"}}),
        button("Crouch", {{"LeftCtrl"}, {"C"}, {"GamepadEast"}}),
        button("Fire", {{"MouseLeft"}, {"GamepadRightTrigger"}}),
        button("Aim", {{"MouseRight"}, {"GamepadLeftTrigger"}}),
        button("Interact", {{"E"}, {"GamepadWest"}}),
        button("Pause", {{"Escape"}, {"GamepadStart"}}),
    };
}

InputSettings::InputSettings()
    : actions(defaultInputActions())
{
}

const InputSettings& inputSettingsOf(const EngineServices& services)
{
    static const InputSettings defaults;
    const Project* project = services.project();
    const SettingsSection* section = project ? project->settings().find(typeNameOf<InputSettings>()) : nullptr;

    return section ? static_cast<const InputSettings&>(*section) : defaults;
}

} // namespace yqlora
