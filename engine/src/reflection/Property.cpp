#include "yqlora/reflection/Property.hpp"

#include <cassert>

namespace yqlora {

Property Property::forNode(std::string name, PropertyType type, NodeGetter getter, NodeSetter setter, PropertyHint hint)
{
    Property property;
    property.m_name = std::move(name);
    property.m_type = type;
    property.m_hint = hint;
    property.m_nodeGet = std::move(getter);
    property.m_nodeSet = std::move(setter);

    return property;
}

Property Property::forComponent(
    std::string name,
    PropertyType type,
    ComponentGetter getter,
    ComponentSetter setter,
    PropertyHint hint
)
{
    Property property;
    property.m_name = std::move(name);
    property.m_type = type;
    property.m_hint = hint;
    property.m_componentGet = std::move(getter);
    property.m_componentSet = std::move(setter);

    return property;
}

Property Property::forSettings(
    std::string name,
    PropertyType type,
    SettingsGetter getter,
    SettingsSetter setter,
    PropertyHint hint
)
{
    Property property;
    property.m_name = std::move(name);
    property.m_type = type;
    property.m_hint = hint;
    property.m_settingsGet = std::move(getter);
    property.m_settingsSet = std::move(setter);

    return property;
}

void Property::setEnumNames(std::vector<std::string> names)
{
    assert(m_type == PropertyType::Enum && "setEnumNames: the property is not an enum");
    m_enumNames = std::move(names);
}

PropertyValue Property::get(const Node& node) const
{
    assert(m_nodeGet && "Property::get: this is a component property");

    return m_nodeGet(node);
}

PropertyValue Property::get(const Component& component) const
{
    assert(m_componentGet && "Property::get: this is a node property");

    return m_componentGet(component);
}

PropertyValue Property::get(const SettingsSection& section) const
{
    assert(m_settingsGet && "Property::get: this is not a settings property");

    return m_settingsGet(section);
}

bool Property::set(Node& node, const PropertyValue& value) const
{
    assert(m_nodeSet && "Property::set: this is a component property");
    if (static_cast<PropertyType>(value.index()) != m_type) {
        return false;
    }
    m_nodeSet(node, value);

    return true;
}

bool Property::set(Component& component, const PropertyValue& value) const
{
    assert(m_componentSet && "Property::set: this is a node property");
    if (static_cast<PropertyType>(value.index()) != m_type) {
        return false;
    }
    m_componentSet(component, value);

    return true;
}

bool Property::set(SettingsSection& section, const PropertyValue& value) const
{
    assert(m_settingsSet && "Property::set: this is not a settings property");
    if (static_cast<PropertyType>(value.index()) != m_type) {
        return false;
    }
    m_settingsSet(section, value);

    return true;
}

} // namespace yqlora
