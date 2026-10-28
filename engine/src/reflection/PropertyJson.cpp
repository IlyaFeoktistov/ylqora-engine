#include "yqlora/reflection/PropertyJson.hpp"

#include "yqlora/core/JsonId.hpp"

#include <algorithm>

namespace yqlora {

namespace {

using Json = nlohmann::json;

std::optional<PropertyValue> enumFromJson(const Json& json, const Property& property)
{
    if (json.is_number_integer()) {
        return EnumValue{json.get<int>()};
    }
    if (!json.is_string()) {
        return std::nullopt;
    }
    const auto& names = property.enumNames();
    const auto it = std::ranges::find(names, json.get<std::string>());
    if (it == names.end()) {
        return std::nullopt;
    }

    return EnumValue{static_cast<int>(it - names.begin())};
}

} // namespace

Json valueToJson(const Property& property, const PropertyValue& value)
{
    return std::visit(
        [&](const auto& v) -> Json {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4>) {
                return vectorToJson(v);
            } else if constexpr (std::is_same_v<T, EnumValue>) {
                // Именем, чтобы порядок значений в C++ можно было менять. Без имени - числом.
                const auto& names = property.enumNames();
                const auto index = static_cast<std::size_t>(v.value);
                if (v.value >= 0 && index < names.size()) {
                    return names[index];
                }

                return v.value;
            } else if constexpr (std::is_same_v<T, NodeRef> || std::is_same_v<T, ResourceRef>) {
                return idToJson(v.id);
            } else {
                return Json(v);
            }
        },
        value
    );
}

std::optional<PropertyValue> valueFromJson(const Json& json, const Property& property)
{
    switch (property.type()) {
    case PropertyType::Bool:
        if (json.is_boolean()) {
            return json.get<bool>();
        }
        break;
    case PropertyType::Int:
        if (json.is_number_integer()) {
            return json.get<int>();
        }
        break;
    case PropertyType::Float:
        if (json.is_number()) {
            return json.get<float>();
        }
        break;
    case PropertyType::String:
        if (json.is_string()) {
            return json.get<std::string>();
        }
        break;
    case PropertyType::Vector2: return vectorFromJson<Vec2>(json);
    case PropertyType::Vector3: return vectorFromJson<Vec3>(json);
    case PropertyType::Vector4: return vectorFromJson<Vec4>(json);
    case PropertyType::Enum: return enumFromJson(json, property);
    case PropertyType::Node:
        if (const auto id = idFromJson(json)) {
            return NodeRef{*id};
        }
        break;
    case PropertyType::Resource:
        if (const auto id = idFromJson(json)) {
            return ResourceRef{*id};
        }
        break;
    }

    return std::nullopt;
}

} // namespace yqlora
