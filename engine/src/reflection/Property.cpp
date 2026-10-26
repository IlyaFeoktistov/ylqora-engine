#include "yqlora/reflection/Property.hpp"

#include <cassert>

namespace yqlora {

PropertyValue Property::get(const Node& node) const
{
    assert(m_forNode && "Property::get: this is a component property");

    return m_get(&node);
}

PropertyValue Property::get(const Component& component) const
{
    assert(!m_forNode && "Property::get: this is a node property");

    return m_get(&component);
}

bool Property::set(Node& node, const PropertyValue& value) const
{
    assert(m_forNode && "Property::set: this is a component property");
    if (static_cast<PropertyType>(value.index()) != m_type) {
        return false;
    }
    m_set(&node, value);

    return true;
}

bool Property::set(Component& component, const PropertyValue& value) const
{
    assert(!m_forNode && "Property::set: this is a node property");
    if (static_cast<PropertyType>(value.index()) != m_type) {
        return false;
    }
    m_set(&component, value);

    return true;
}

} // namespace yqlora
