#pragma once

namespace yqlora {

class TypeRegistry;

// Разделы настроек проекта, которые не принадлежат ни одной подсистеме.
void registerProjectTypes(TypeRegistry& registry);

} // namespace yqlora
