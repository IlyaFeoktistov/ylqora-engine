#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/reflection/Record.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace yqlora {

class EngineServices;

enum class InputActionKind : std::uint8_t {
    // Нажата, зажата, отпущена.
    Button,
    // Значение от -1 до 1: сумма привязок со своими множителями.
    Axis,
};

// Привязка действия к органу управления: имя клавиши или кнопки мыши (см.
// InputControl). У оси нажатый орган даёт scale: две клавиши с 1 и -1 - ось.
struct InputBinding {
    std::string control;
    float scale = 1.0f;
};

// Действие: скрипты спрашивают его по имени, а не клавишу, поэтому
// переназначение клавиш не трогает код игры.
struct InputAction {
    std::string name;
    InputActionKind kind = InputActionKind::Button;
    std::vector<InputBinding> bindings;
};

// Раздел Input настроек проекта: действия ввода и их привязки.
class YQLORA_API InputSettings final : public SettingsSection {
public:
    YQLORA_TYPE(InputSettings, SettingsSection)

    std::vector<InputAction> actions;
};

// Раздел Input открытого проекта, а без проекта - пустой.
YQLORA_API const InputSettings& inputSettingsOf(const EngineServices& services);

} // namespace yqlora

YQLORA_RECORD(
    yqlora::InputBinding,
    field("control", &yqlora::InputBinding::control, PropertyHint::InputControl),
    field("scale", &yqlora::InputBinding::scale)
)

YQLORA_RECORD(
    yqlora::InputAction,
    field("name", &yqlora::InputAction::name),
    field("kind", &yqlora::InputAction::kind, PropertyHint::None, {"Button", "Axis"}),
    field("bindings", &yqlora::InputAction::bindings)
)
