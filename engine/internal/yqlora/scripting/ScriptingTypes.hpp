#pragma once

namespace yqlora {

class TypeRegistry;

// База скриптов на C++ (Script). Типы сцены должны быть уже зарегистрированы.
void registerScriptingTypes(TypeRegistry& registry);

} // namespace yqlora
