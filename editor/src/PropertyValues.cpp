#include "yqlora/editor/PropertyValues.hpp"

#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/reflection/PropertyJson.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/Node.hpp"

#include <algorithm>

namespace yqlora::editor {

std::optional<PropertyValue> defaultValue(const TypeRegistry& registry, const Node& node, const Property& property)
{
    const TypeInfo* type = registry.find(node.typeName());
    const auto fresh = type ? type->createNode() : nullptr;

    return fresh ? std::optional(property.get(*fresh)) : std::nullopt;
}

std::optional<PropertyValue> defaultValue(
    const TypeRegistry& registry,
    const Component& component,
    const Property& property
)
{
    const TypeInfo* type = registry.find(component.typeName());
    if (!type) {
        return std::nullopt;
    }
    if (const auto fresh = type->createComponent()) {
        return property.get(*fresh);
    }
    const Node* owner = component.owner();
    const TypeInfo* ownerType = owner ? registry.find(owner->typeName()) : nullptr;
    const auto freshOwner = ownerType ? ownerType->createNode() : nullptr;
    if (!freshOwner) {
        return std::nullopt;
    }
    const auto& components = freshOwner->components();
    const auto it = std::ranges::find_if(components, [&](const auto& candidate) {
        return candidate->typeName() == component.typeName();
    });

    return it != components.end() ? std::optional(property.get(**it)) : std::nullopt;
}

std::optional<PropertyValue> defaultValue(
    const TypeRegistry& registry,
    const SettingsSection& section,
    const Property& property
)
{
    const TypeInfo* type = registry.find(section.typeName());
    const auto fresh = type ? type->createSettings() : nullptr;

    return fresh ? std::optional(property.get(*fresh)) : std::nullopt;
}

std::string valueToText(const Property& property, const PropertyValue& value)
{
    return valueToJson(property, value).dump();
}

std::optional<PropertyValue> valueFromText(const Property& property, std::string_view text)
{
    const auto json = nlohmann::json::parse(text, nullptr, false);
    // В строку годится и обычный текст из другой программы, без кавычек JSON.
    if (property.type() == PropertyType::String && !json.is_string()) {
        return PropertyValue{std::string(text)};
    }
    if (json.is_discarded()) {
        return std::nullopt;
    }

    return valueFromJson(json, property);
}

std::string numberToText(float value)
{
    return nlohmann::json(value).dump();
}

std::optional<float> numberFromText(std::string_view text)
{
    const auto json = nlohmann::json::parse(text, nullptr, false);

    return json.is_number() ? std::optional(json.get<float>()) : std::nullopt;
}

std::span<float> vectorComponents(PropertyValue& value)
{
    return std::visit(
        [](auto& v) -> std::span<float> {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4>) {
                return {&v.x, static_cast<std::size_t>(T::length())};
            } else {
                return {};
            }
        },
        value
    );
}

} // namespace yqlora::editor
