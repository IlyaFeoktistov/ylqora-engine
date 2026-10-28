#include "yqlora/scene/ExecutionOrder.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

namespace yqlora {

int executionOrderOf(const EngineServices& /*services*/, const TypeInfo& type)
{
    // Раздела Execution Order в настройках проекта ещё нет: только значение из кода.
    return type.executionOrder();
}

} // namespace yqlora
