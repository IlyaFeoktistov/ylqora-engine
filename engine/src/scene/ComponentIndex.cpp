#include "yqlora/scene/ComponentIndex.hpp"

#include "yqlora/scene/Component.hpp"

namespace yqlora {

void ComponentIndex::add(Component& component)
{
    m_byType[std::string(component.typeName())].push_back(&component);
}

void ComponentIndex::remove(Component& component)
{
    if (const auto it = m_byType.find(component.typeName()); it != m_byType.end()) {
        std::erase(it->second, &component);
    }
}

std::span<Component* const> ComponentIndex::ofType(std::string_view typeName) const
{
    const auto it = m_byType.find(typeName);
    if (it == m_byType.end()) {
        return {};
    }

    return it->second;
}

} // namespace yqlora
