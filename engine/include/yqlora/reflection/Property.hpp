#pragma once

#include "yqlora/math/Math.hpp"

#include <functional>
#include <string>
#include <type_traits>
#include <variant>

namespace yqlora {

class Node;
class Component;

// Значение свойства, которое умеют показывать инспектор и сохранять файлы сцен.
// Порядок типов совпадает с PropertyType.
using PropertyValue = std::variant<bool, int, float, std::string, Vec2, Vec3, Vec4>;

enum class PropertyType {
    Bool,
    Int,
    Float,
    String,
    Vector2,
    Vector3,
    Vector4,
};

static_assert(std::variant_size_v<PropertyValue> == 7, "PropertyType must list every PropertyValue type");

// PropertyType для типа C++. Для типа, которого нет в PropertyValue, не компилируется.
template <typename T>
constexpr PropertyType propertyTypeOf()
{
    if constexpr (std::is_same_v<T, bool>) {
        return PropertyType::Bool;
    } else if constexpr (std::is_same_v<T, int>) {
        return PropertyType::Int;
    } else if constexpr (std::is_same_v<T, float>) {
        return PropertyType::Float;
    } else if constexpr (std::is_same_v<T, std::string>) {
        return PropertyType::String;
    } else if constexpr (std::is_same_v<T, Vec2>) {
        return PropertyType::Vector2;
    } else if constexpr (std::is_same_v<T, Vec3>) {
        return PropertyType::Vector3;
    } else if constexpr (std::is_same_v<T, Vec4>) {
        return PropertyType::Vector4;
    } else {
        static_assert(sizeof(T) == 0, "Property type is not supported, see PropertyValue");
    }
}

// Как показывать свойство в инспекторе, если обычного поля для его типа мало.
enum class PropertyHint {
    None,
    Color, // Vec4 как цвет RGBA с палитрой
    Mesh,  // строка с именем меша: выбор из доступных мешей
};

// Свойство узла или компонента: имя, тип и доступ к значению. Создаётся
// через TypeBuilder при регистрации типа в TypeRegistry.
class Property {
public:
    const std::string& name() const
    {
        return m_name;
    }

    PropertyType type() const
    {
        return m_type;
    }

    PropertyHint hint() const
    {
        return m_hint;
    }

    // Объект должен быть того типа, для которого зарегистрировано свойство, или его наследником.
    PropertyValue get(const Node& node) const;
    PropertyValue get(const Component& component) const;

    // false, если тип значения не совпадает с типом свойства.
    bool set(Node& node, const PropertyValue& value) const;
    bool set(Component& component, const PropertyValue& value) const;

private:
    template <typename Class, typename Root>
    friend class TypeBuilder;

    // Указатель на объект приходит как Root (Node или Component), к своему типу
    // его приводит функция, созданная в TypeBuilder.
    using Getter = std::function<PropertyValue(const void*)>;
    using Setter = std::function<void(void*, const PropertyValue&)>;

    std::string m_name;
    PropertyType m_type = PropertyType::Bool;
    PropertyHint m_hint = PropertyHint::None;
    bool m_forNode = false;
    Getter m_get;
    Setter m_set;
};

} // namespace yqlora
