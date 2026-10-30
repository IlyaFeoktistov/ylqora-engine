#pragma once

namespace yqlora {

class TypeRegistry;

// Раздел Input настроек проекта. Корень разделов должен быть уже зарегистрирован.
void registerInputTypes(TypeRegistry& registry);

} // namespace yqlora
