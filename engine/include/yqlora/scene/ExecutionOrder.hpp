#pragma once

#include "yqlora/core/Export.hpp"

namespace yqlora {

class EngineServices;
class TypeInfo;

// Порядок выполнения типа компонента внутри фаз кадра (меньше - раньше).
// Единственное место правила: значение из раздела Execution Order настроек
// проекта, если оно там есть, иначе значение по умолчанию из кода
// (TypeBuilder::executionOrder).
YQLORA_API int executionOrderOf(const EngineServices& services, const TypeInfo& type);

} // namespace yqlora
