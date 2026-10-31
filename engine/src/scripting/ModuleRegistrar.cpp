#include "yqlora/scripting/ModuleRegistrar.hpp"

namespace yqlora {

ModuleRegistrar::ModuleRegistrar(TypeRegistry& registry, std::string module)
    : m_registry(registry)
    , m_scope(registry, std::move(module))
{
}

bool ModuleRegistrar::accept(std::string_view name)
{
    const TypeInfo* taken = m_registry.find(name);
    if (!taken) {
        return true;
    }
    m_rejected.push_back({std::string(name), taken->module()});

    return false;
}

} // namespace yqlora
