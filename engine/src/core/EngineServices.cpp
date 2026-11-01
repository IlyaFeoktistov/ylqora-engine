#include "yqlora/core/EngineServices.hpp"

#include "yqlora/input/Input.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/resource/ResourceCache.hpp"

namespace yqlora {

EngineServices::EngineServices(const Input& input, const TypeRegistry& registry)
    : m_input(input)
    , m_registry(registry)
    , m_resources(std::make_shared<ResourceCache>())
{
}

EngineServices::EngineServices(const Input& input, const EngineServices& shared)
    : m_input(input)
    , m_registry(shared.m_registry)
    , m_project(shared.m_project)
    , m_resources(shared.m_resources)
{
}

EngineServices::~EngineServices() = default;

void EngineServices::setProject(const Project* project)
{
    m_project = project;
    m_resources->setProject(project);
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
