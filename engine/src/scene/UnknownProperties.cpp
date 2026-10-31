#include "yqlora/scene/UnknownProperties.hpp"

#include "yqlora/scene/Component.hpp"

namespace yqlora {

nlohmann::json UnknownProperties::of(const Component& component)
{
    return component.m_unknownProperties.empty() ? nlohmann::json::object()
                                                 : nlohmann::json::parse(component.m_unknownProperties);
}

void UnknownProperties::set(Component& component, const nlohmann::json& properties)
{
    component.m_unknownProperties = properties.empty() ? std::string() : properties.dump();
}

} // namespace yqlora
