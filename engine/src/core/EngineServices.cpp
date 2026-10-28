#include "yqlora/core/EngineServices.hpp"

#include "yqlora/input/Input.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

namespace yqlora {

EngineServices::EngineServices(const Input& input, const TypeRegistry& registry)
    : m_input(input)
    , m_registry(registry)
{
}

const EngineServices& EngineServices::none()
{
    // Неизменяемый пустой объект, а не общее состояние: изменить его нельзя.
    static const Input input;
    static const TypeRegistry registry;
    static const EngineServices services(input, registry);

    return services;
}

} // namespace yqlora
