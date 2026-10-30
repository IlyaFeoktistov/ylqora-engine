#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/input/Key.hpp"

#include <optional>
#include <string_view>
#include <variant>

namespace yqlora {

class Input;

// Орган управления, к которому привязывают действие ввода: клавиша или кнопка
// мыши. В файлах и в редакторе - по имени (kKeyNames, kMouseButtonNames).
using InputControl = std::variant<Key, MouseButton>;

// nullopt, если такого имени нет.
YQLORA_API std::optional<InputControl> controlFromName(std::string_view name);
YQLORA_API std::string_view controlName(const InputControl& control);

// Состояние органа управления в этом кадре.
YQLORA_API bool isControlDown(const Input& input, const InputControl& control);
YQLORA_API bool wasControlPressed(const Input& input, const InputControl& control);

} // namespace yqlora
