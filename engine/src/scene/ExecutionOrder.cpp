#include "yqlora/scene/ExecutionOrder.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/ExecutionOrderSettings.hpp"

namespace yqlora {

int executionOrderOf(const EngineServices& services, const TypeInfo& type)
{
    const Project* project = services.project();
    const auto* settings = project ? static_cast<const ExecutionOrderSettings*>(
                                         project->settings().find(typeNameOf<ExecutionOrderSettings>())
                                     )
                                   : nullptr;
    const auto overridden = settings ? settings->orderOf(type.name()) : std::nullopt;

    return overridden.value_or(type.executionOrder());
}

} // namespace yqlora
