#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/input/Key.hpp"

#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace yqlora {

class Input;

// Орган управления, к которому привязывают действие ввода: клавиша, кнопка
// или ось мыши, кнопка или ось геймпада. В файлах и в редакторе - по имени
// (kKeyNames, kMouseButtonNames, kMouseAxisNames, kGamepadButtonNames, kGamepadAxisNames).
using InputControl = std::variant<Key, MouseButton, MouseAxis, GamepadButton, GamepadAxis>;

// Все органы управления по порядку: клавиши, кнопки и оси мыши, кнопки и оси
// геймпада. Для списка выбора и для задания привязки нажатием.
YQLORA_API std::vector<InputControl> allInputControls();

// nullopt, если такого имени нет.
YQLORA_API std::optional<InputControl> controlFromName(std::string_view name);
YQLORA_API std::string_view controlName(const InputControl& control);

// Значение в этом кадре: у клавиш и кнопок 1 или 0, у стиков от -1 до 1, у
// курков от 0 до 1, у мыши - сдвиг в точках (вверх и вправо - плюс) и шаги
// колеса, без предела.
YQLORA_API float controlValue(const Input& input, const InputControl& control);

// Ось мыши - сдвиг за кадр, а не положение: её значение не ограничено.
YQLORA_API bool isUnbounded(const InputControl& control);

// Задан ли орган в этом кадре: нажатие клавиши или кнопки, отклонение оси
// геймпада дальше половины. Сдвиг мыши не считается: мышь двигают, чтобы
// дотянуться до кнопки. Так редактор задаёт привязку нажатием.
YQLORA_API bool wasControlActivated(const Input& input, const InputControl& control);

} // namespace yqlora
