#pragma once

#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/Node.hpp"

#include <cassert>
#include <concepts>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <typeindex>
#include <utility>
#include <vector>

namespace yqlora {

enum class TypeKind {
    Node,
    Component,
};

// Описание типа узла или компонента: по нему редактор строит меню создания
// и инспектор, а файлы сцен создают объекты по имени типа.
class TypeInfo {
public:
    const std::string& name() const
    {
        return m_name;
    }

    // Пусто у Node и у компонентов, унаследованных прямо от Component.
    const std::string& baseName() const
    {
        return m_baseName;
    }

    TypeKind kind() const
    {
        return m_kind;
    }

    // Встроенные компоненты (Transform) создаёт сам узел, поэтому они не создаваемые.
    bool creatable() const
    {
        return m_createNode != nullptr || m_createComponent != nullptr;
    }

    // Ключ перевода с описанием типа для подсказки в редакторе.
    std::string descriptionKey() const
    {
        return "type." + m_name + ".description";
    }

    // Только свойства этого типа, без базовых. Все вместе: TypeRegistry::properties().
    std::span<const Property> ownProperties() const
    {
        return m_properties;
    }

    // nullptr, если это не узел (не компонент) или тип нельзя создать.
    std::unique_ptr<Node> createNode() const;
    std::unique_ptr<Component> createComponent() const;

private:
    friend class TypeRegistry;
    template <typename Class, typename Root>
    friend class TypeBuilder;

    std::string m_name;
    std::string m_baseName;
    TypeKind m_kind = TypeKind::Node;
    std::function<std::unique_ptr<Node>()> m_createNode;
    std::function<std::unique_ptr<Component>()> m_createComponent;
    std::vector<Property> m_properties;
};

// Добавляет свойства к только что зарегистрированному типу:
//   registry.registerNode<Camera3D, Node3D>("Camera3D")
//       .property("fov", &Camera3D::fovDegrees, &Camera3D::setFovDegrees);
template <typename Class, typename Root>
class TypeBuilder {
public:
    explicit TypeBuilder(TypeInfo& info)
        : m_info(info)
    {
    }

    // Свойство через getter и setter: методы класса или любые вызываемые объекты
    // вида Value(const Class&) и void(Class&, Value).
    template <typename Getter, typename Setter>
    TypeBuilder& property(std::string name, Getter getter, Setter setter, PropertyHint hint = PropertyHint::None)
    {
        using Value = std::remove_cvref_t<std::invoke_result_t<Getter, const Class&>>;

        Property property;
        property.m_name = std::move(name);
        property.m_type = propertyTypeOf<Value>();
        property.m_hint = hint;
        property.m_forNode = std::is_same_v<Root, Node>;
        property.m_get = [getter](const void* object) -> PropertyValue {
            return PropertyValue(std::in_place_type<Value>, std::invoke(getter, cast(object)));
        };
        property.m_set = [setter](void* object, const PropertyValue& value) {
            std::invoke(setter, cast(object), std::get<Value>(value));
        };
        m_info.m_properties.push_back(std::move(property));

        return *this;
    }

    // Свойство-поле, когда getter и setter не нужны: .property("radius", &SphereShape::radius);
    template <typename Value>
    TypeBuilder& property(std::string name, Value Class::*field, PropertyHint hint = PropertyHint::None)
    {
        const auto getter = [field](const Class& object) -> const Value& {
            return object.*field;
        };
        const auto setter = [field](Class& object, const Value& value) {
            object.*field = value;
        };

        return property(std::move(name), getter, setter, hint);
    }

private:
    static const Class& cast(const void* object)
    {
        return static_cast<const Class&>(*static_cast<const Root*>(object));
    }

    static Class& cast(void* object)
    {
        return static_cast<Class&>(*static_cast<Root*>(object));
    }

    TypeInfo& m_info;
};

// Список типов узлов и компонентов, доступных редактору и файлам сцен.
// Базовый тип регистрируется раньше наследника. Типы движка добавляет
// registerEngineTypes(), игра регистрирует свои рядом.
class TypeRegistry {
public:
    TypeRegistry() = default;

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    template <std::derived_from<Node> T, std::derived_from<Node> Base = Node>
    TypeBuilder<T, Node> registerNode(std::string name)
    {
        TypeInfo& info = add<T, Base>(std::move(name), TypeKind::Node);
        if constexpr (std::is_default_constructible_v<T>) {
            info.m_createNode = [] {
                return std::unique_ptr<Node>(std::make_unique<T>());
            };
        }

        return TypeBuilder<T, Node>(info);
    }

    template <std::derived_from<Component> T, std::derived_from<Component> Base = Component>
    TypeBuilder<T, Component> registerComponent(std::string name)
    {
        TypeInfo& info = add<T, Base>(std::move(name), TypeKind::Component);
        if constexpr (std::is_default_constructible_v<T>) {
            info.m_createComponent = [] {
                return std::unique_ptr<Component>(std::make_unique<T>());
            };
        }

        return TypeBuilder<T, Component>(info);
    }

    // nullptr, если тип не зарегистрирован.
    const TypeInfo* find(std::string_view name) const;

    template <typename T>
    const TypeInfo* find() const
    {
        const auto it = m_byCppType.find(std::type_index(typeid(T)));

        return it != m_byCppType.end() ? it->second : nullptr;
    }

    // Все типы в порядке регистрации.
    std::span<const std::unique_ptr<TypeInfo>> types() const
    {
        return m_types;
    }

    // Свойства типа вместе с базовыми, от Node к наследнику.
    std::vector<const Property*> properties(const TypeInfo& type) const;

private:
    template <typename T, typename Base>
    TypeInfo& add(std::string name, TypeKind kind)
    {
        assert(!find(name) && "TypeRegistry: type name is already registered");
        assert(!find<T>() && "TypeRegistry: C++ type is already registered");

        auto info = std::make_unique<TypeInfo>();
        info->m_name = std::move(name);
        info->m_kind = kind;
        if constexpr (!std::is_same_v<T, Base> && !std::is_same_v<Base, Component>) {
            const TypeInfo* base = find<Base>();
            assert(base && "TypeRegistry: register the base type first");
            info->m_baseName = base->name();
        }

        TypeInfo& ref = *info;
        m_byName.emplace(ref.m_name, &ref);
        m_byCppType.emplace(std::type_index(typeid(T)), &ref);
        m_types.push_back(std::move(info));

        return ref;
    }

    std::vector<std::unique_ptr<TypeInfo>> m_types;
    std::map<std::string, TypeInfo*, std::less<>> m_byName;
    std::map<std::type_index, TypeInfo*> m_byCppType;
};

// Типы движка: Node, Node3D, Camera3D, MeshInstance3D, Transform3D.
void registerEngineTypes(TypeRegistry& registry);

} // namespace yqlora
