#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/resource/ResourceId.hpp"
#include "yqlora/scene/NodeId.hpp"

#include <bit>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace yqlora {

class Node;
class Component;
class SettingsSection;

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

// 32 бита без знака (маска слоёв) - как int с теми же битами.
template <>
struct PropertyStorage<std::uint32_t> {
    using Type = int;

    static int store(std::uint32_t value)
    {
        return std::bit_cast<int>(value);
    }

    static std::uint32_t load(int value)
    {
        return std::bit_cast<std::uint32_t>(value);
    }
};

// Строка без владения (параметр метода) - как std::string.
template <>
struct PropertyStorage<std::string_view> {
    using Type = std::string;

    static std::string store(std::string_view value)
    {
        return std::string(value);
    }

    static std::string_view load(const std::string& value)
    {
        return value;
    }
};

// Указатель на узел (результат метода) - как ссылка на узел, nullptr - пустая
// ссылка. Обратно только через сцену, поэтому load нет: параметром метода
// указатель на узел быть не может.
template <typename T>
    requires std::derived_from<std::remove_const_t<T>, Node>
struct PropertyStorage<T*> {
    using Type = NodeRef;

    static NodeRef store(const T* node)
    {
        return node ? NodeRef{node->id()} : NodeRef{};
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
    Color,  // Vec4 как цвет RGBA с палитрой
    Mesh,   // ссылка на ресурс меша: выбор из доступных мешей
    Angle,  // градусы: при перетаскивании значение меняется медленнее
    Layer,  // int: один слой узла из списка
    Layers, // int: набор слоёв (LayerMask), флажок на каждый
    Scene,  // ссылка на ресурс сцены: выбор из сцен проекта
};

// Свойство узла, компонента или раздела настроек проекта: имя, тип и доступ к значению. Доступ к
// значению - функции над PropertyValue, поэтому свойство может объявить и
// C++-класс (ClassTypeBuilder), и тип, описанный во время выполнения
// (скрипт Luau, граф визуального скрипта).
class YQLORA_API Property {
public:
    using NodeGetter = std::function<PropertyValue(const Node&)>;
    using NodeSetter = std::function<void(Node&, const PropertyValue&)>;
    using ComponentGetter = std::function<PropertyValue(const Component&)>;
    using ComponentSetter = std::function<void(Component&, const PropertyValue&)>;
    using SettingsGetter = std::function<PropertyValue(const SettingsSection&)>;
    using SettingsSetter = std::function<void(SettingsSection&, const PropertyValue&)>;

    // Setter получает значение уже проверенного типа type.
    static Property forNode(
        std::string name,
        PropertyType type,
        NodeGetter getter,
        NodeSetter setter,
        PropertyHint hint = PropertyHint::None
    );
    static Property forComponent(
        std::string name,
        PropertyType type,
        ComponentGetter getter,
        ComponentSetter setter,
        PropertyHint hint = PropertyHint::None
    );
    static Property forSettings(
        std::string name,
        PropertyType type,
        SettingsGetter getter,
        SettingsSetter setter,
        PropertyHint hint = PropertyHint::None
    );

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
    PropertyValue get(const SettingsSection& section) const;

    // false, если тип значения не совпадает с типом свойства.
    bool set(Node& node, const PropertyValue& value) const;
    bool set(Component& component, const PropertyValue& value) const;
    bool set(SettingsSection& section, const PropertyValue& value) const;

    // Имена значений перечисления по порядку, см. enumNames.
    void setEnumNames(std::vector<std::string> names);

private:
    Property() = default;

    std::string m_name;
    PropertyType m_type = PropertyType::Bool;
    PropertyHint m_hint = PropertyHint::None;
    std::vector<std::string> m_enumNames;
    // Своё у каждого вида объектов: объект одного вида не передать свойству другого.
    NodeGetter m_nodeGet;
    NodeSetter m_nodeSet;
    ComponentGetter m_componentGet;
    ComponentSetter m_componentSet;
    SettingsGetter m_settingsGet;
    SettingsSetter m_settingsSet;
};

} // namespace yqlora
