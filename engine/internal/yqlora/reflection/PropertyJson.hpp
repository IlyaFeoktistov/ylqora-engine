#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <optional>

namespace yqlora {

// Значения свойств в JSON и обратно: одно правило для всех файлов движка,
// где лежат свойства (сцены, потом настройки проекта).

// Вектор glm - массивом чисел.
template <typename Vec>
nlohmann::json vectorToJson(const Vec& vec)
{
    nlohmann::json result = nlohmann::json::array();
    for (glm::length_t i = 0; i < Vec::length(); ++i) {
        result.push_back(vec[i]);
    }

    return result;
}

template <typename Vec>
std::optional<Vec> vectorFromJson(const nlohmann::json& json)
{
    if (!json.is_array() || json.size() != static_cast<std::size_t>(Vec::length())) {
        return std::nullopt;
    }
    Vec result;
    for (glm::length_t i = 0; i < Vec::length(); ++i) {
        const nlohmann::json& item = json[static_cast<std::size_t>(i)];
        if (!item.is_number()) {
            return std::nullopt;
        }
        result[i] = item.get<float>();
    }

    return result;
}

YQLORA_API nlohmann::json valueToJson(const Property& property, const PropertyValue& value);

// nullopt, если значение не того типа или такого значения перечисления нет.
YQLORA_API std::optional<PropertyValue> valueFromJson(const nlohmann::json& json, const Property& property);

template <typename Object>
nlohmann::json propertiesToJson(const TypeRegistry& registry, const TypeInfo& type, const Object& object)
{
    nlohmann::json result = nlohmann::json::object();
    for (const Property* property : registry.properties(type)) {
        result[property->name()] = valueToJson(*property, property->get(object));
    }

    return result;
}

// Отсутствующие свойства остаются значениями по умолчанию, лишние игнорируются:
// когда в коде игры добавляют или убирают поле, уже сохранённые сцены открываются.
template <typename Object>
void propertiesFromJson(const TypeRegistry& registry, const TypeInfo& type, Object& object, const nlohmann::json& json)
{
    if (!json.is_object()) {
        return;
    }
    for (const Property* property : registry.properties(type)) {
        const auto it = json.find(property->name());
        if (it == json.end()) {
            continue;
        }
        const auto value = valueFromJson(*it, *property);
        if (!value) {
            log::warn("Property '{}.{}' has a wrong value, skipped", type.name(), property->name());
            continue;
        }
        property->set(object, *value);
    }
}

} // namespace yqlora
