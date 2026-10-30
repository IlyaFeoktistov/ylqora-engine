#pragma once

#include "yqlora/core/Export.hpp"

namespace yqlora {

class TypeRegistry;

// Типы движка: каждая подсистема регистрирует свои, здесь только порядок
// подсистем. Базовые типы раньше наследников, системы сцены вызываются в
// порядке регистрации.
YQLORA_API void registerEngineTypes(TypeRegistry& registry);

} // namespace yqlora
