#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/resource/ResourceId.hpp"
#include "yqlora/scene/NodeId.hpp"

#include <cstddef>
#include <functional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace yqlora {

class Node;
class Component;

// Значение перечисления. В файлах сцен пишется именем, а не числом, поэтому
// порядок значений в C++ можно менять.
struct EnumValue {
    int value = 0;

    bool operator==(const EnumValue&) const = default;
};

// Ссылка на узел той же сцены. Если узел удалили, ссылка никуда не ведёт.
struct NodeRef {
    NodeId id = 0;

    bool operator==(const NodeRef&) const = default;
};

struct ResourceRef {
    ResourceId id = 0;

    bool operator==(const ResourceRef&) const = default;
};

// Значение свойства, которое умеют показывать инспектор и сохранять файлы сцен.
// Порядок типов совпадает с PropertyType.
using PropertyValue = std::variant<bool, int, float, std::string, Vec2, Vec3, Vec4, EnumValue, NodeRef, ResourceRef>;

enum class PropertyType {
    Bool,
    Int,
    Float,
    String,
    Vector2,
    Vector3,
    Vector4,
    Enum,
    Node,
    Resource,
};

static_assert(std::variant_size_v<PropertyValue> == 10, "PropertyType must list every PropertyValue type");

// Как значение типа C++ хранится в PropertyValue. Обычно как есть,
// а любое перечисление C++ - как EnumValue.
template <typename T>
struct PropertyStorage {
    using Type = T;

    static const T& store(const T& value)
    {
        return value;
    }

    static const T& load(const T& value)
    {
        return value;
    }
};

template <typename T>
    requires std::is_enum_v<T>
struct PropertyStorage<T> {
    using Type = EnumValue;

    static EnumValue store(T value)
    {
        return {static_cast<int>(value)};
    }

    static T load(const EnumValue& value)
    {
        return static_cast<T>(value.value);
    }
};

namespace detail {

template <typename T, typename Variant>
struct VariantIndex;

template <typename T, typename... Types>
struct VariantIndex<T, std::variant<Types...>> {
    static constexpr std::size_t find()
    {
        constexpr bool matches[] = {std::is_same_v<T, Types>...};
        for (std::size_t i = 0; i < sizeof...(Types); ++i) {
            if (matches[i]) {
                return i;
            }
        }

        return sizeof...(Types);
    }

    static constexpr std::size_t value = find();
    static_assert(value < sizeof...(Types), "Property type is not supported, see PropertyValue");
};

} // namespace detail

// PropertyType для типа C++: номер его вида хранения в PropertyValue.
// Для типа, которого нет в PropertyValue, не компилируется.
template <typename T>
constexpr PropertyType propertyTypeOf()
{
    using Stored = typename PropertyStorage<T>::Type;

    return static_cast<PropertyType>(detail::VariantIndex<Stored, PropertyValue>::value);
}

// Как показывать свойство в инспекторе, если обычного поля для его типа мало.
enum class PropertyHint {
    None,
    Color, // Vec4 как цвет RGBA с палитрой
    Mesh,  // строка с именем меша: выбор из доступных мешей
    Angle, // градусы: при перетаскивании значение меняется медленнее
};

// Свойство узла или компонента: имя, тип и доступ к значению. Создаётся
// через TypeBuilder при регистрации типа в TypeRegistry.
class YQLORA_API Property {
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

    // Имена значений перечисления по порядку. Пусто у остальных типов.
    const std::vector<std::string>& enumNames() const
    {
        return m_enumNames;
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
    std::vector<std::string> m_enumNames;
    bool m_forNode = false;
    Getter m_get;
    Setter m_set;
};

} // namespace yqlora
