#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/reflection/TypeInfo.hpp"
#include "yqlora/reflection/TypeName.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/SceneSystem.hpp"

#include <cassert>
#include <concepts>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace yqlora {

// Описание типа, общее для любого способа регистрации: свойства значениями и
// правила компонента. Self - конкретный строитель: методы возвращают его,
// чтобы в цепочке вызовов не терялись его собственные методы.
template <typename Self>
class BasicTypeBuilder {
public:
    // Свойство, уже описанное значениями (Property::forNode, forComponent).
    Self& property(Property property)
    {
        m_info.m_properties.push_back(std::move(property));

        return self();
    }

    // Имена значений перечисления, которое добавили последним свойством:
    //   .property("clear", &Camera3D::clear, &Camera3D::setClear)
    //   .enumNames({"ColorAndDepth", "DepthOnly", "Nothing"})
    Self& enumNames(std::initializer_list<std::string_view> names)
    {
        assert(!m_info.m_properties.empty() && "enumNames: add the enum property first");
        m_info.m_properties.back().setEnumNames(std::vector<std::string>(names.begin(), names.end()));

        return self();
    }

    // Компонент живёт только на узлах типа nodeType и его наследников.
    Self& requiresNode(std::string nodeType)
    {
        assertComponent();
        m_info.m_requiredNode = std::move(nodeType);

        return self();
    }

    // На том же узле нужен компонент типа componentType (или его наследника).
    // Редактор добавляет его сам, а снять его нельзя, пока от него зависят.
    Self& requiresComponent(std::string componentType)
    {
        assertComponent();
        m_info.m_requiredComponents.push_back(std::move(componentType));

        return self();
    }

    // Порядок выполнения в фазах кадра по умолчанию (меньше - раньше, по
    // умолчанию 0). Настройки проекта могут его переопределить, см. executionOrderOf.
    Self& executionOrder(int order)
    {
        assertComponent();
        m_info.m_executionOrder = order;

        return self();
    }

    // Несколько компонентов этого типа на одном узле. По умолчанию один.
    Self& allowMultiple()
    {
        assertComponent();
        m_info.m_allowMultiple = true;

        return self();
    }

protected:
    explicit BasicTypeBuilder(TypeInfo& info)
        : m_info(info)
    {
    }

    Self& self()
    {
        return static_cast<Self&>(*this);
    }

    void assertComponent() const
    {
        assert(m_info.m_kind == TypeKind::Component && "TypeBuilder: this rule is for components only");
    }

    TypeInfo& m_info;
};

// Тип, описанный во время выполнения, без своего C++-класса: скрипт Luau,
// граф визуального скрипта. Создание и фазы задаются явно:
//   registry.registerType("Player", TypeKind::Component, "Script")
//       .createComponent([script] { return std::make_unique<LuauScript>(script); })
//       .phases(phaseBit(LifecyclePhase::Update))
//       .property(Property::forComponent("speed", PropertyType::Float, getSpeed, setSpeed));
class YQLORA_API TypeBuilder final : public BasicTypeBuilder<TypeBuilder> {
public:
    explicit TypeBuilder(TypeInfo& info)
        : BasicTypeBuilder(info)
    {
    }

    TypeBuilder& createNode(std::function<std::unique_ptr<Node>()> factory);
    TypeBuilder& createComponent(std::function<std::unique_ptr<Component>()> factory);
    TypeBuilder& createSystem(std::function<std::unique_ptr<SceneSystem>(Scene&)> factory);

    // Фазы кадра, в которых сцена вызывает компоненты типа.
    TypeBuilder& phases(LifecyclePhases phases);
};

// Тип - C++-класс: свойства из его методов и полей, правила по C++-типам.
// Сам ничего не хранит, переводит члены класса в значения BasicTypeBuilder:
//   registry.registerNode<Camera3D>()
//       .property("fov", &Camera3D::fovDegrees, &Camera3D::setFovDegrees);
template <typename Class, typename Root>
class ClassTypeBuilder final : public BasicTypeBuilder<ClassTypeBuilder<Class, Root>> {
    using Base = BasicTypeBuilder<ClassTypeBuilder<Class, Root>>;

public:
    explicit ClassTypeBuilder(TypeInfo& info)
        : Base(info)
    {
    }

    using Base::property;
    using Base::requiresComponent;
    using Base::requiresNode;

    // Свойство через getter и setter: методы класса или любые вызываемые объекты
    // вида Value(const Class&) и void(Class&, Value).
    template <typename Getter, typename Setter>
    ClassTypeBuilder& property(std::string name, Getter getter, Setter setter, PropertyHint hint = PropertyHint::None)
        requires(!std::same_as<Root, SceneSystem>)
    {
        using Value = std::remove_cvref_t<std::invoke_result_t<Getter, const Class&>>;
        using Storage = PropertyStorage<Value>;
        using Stored = typename Storage::Type;
        const PropertyType type = propertyTypeOf<Value>();

        const auto get = [getter](const Root& object) -> PropertyValue {
            return PropertyValue(
                std::in_place_type<Stored>,
                Storage::store(std::invoke(getter, static_cast<const Class&>(object)))
            );
        };
        const auto set = [setter](Root& object, const PropertyValue& value) {
            std::invoke(setter, static_cast<Class&>(object), Storage::load(std::get<Stored>(value)));
        };
        if constexpr (std::same_as<Root, Node>) {
            return property(Property::forNode(std::move(name), type, get, set, hint));
        } else {
            return property(Property::forComponent(std::move(name), type, get, set, hint));
        }
    }

    // Свойство-поле, когда getter и setter не нужны: .property("radius", &SphereShape::radius);
    template <typename Value>
    ClassTypeBuilder& property(std::string name, Value Class::* field, PropertyHint hint = PropertyHint::None)
        requires(!std::same_as<Root, SceneSystem>)
    {
        const auto getter = [field](const Class& object) -> const Value& {
            return object.*field;
        };
        const auto setter = [field](Class& object, const Value& value) {
            object.*field = value;
        };

        return property(std::move(name), getter, setter, hint);
    }

    //   registry.registerComponent<MeshRenderer>().requiresNode<Node3D>();
    template <std::derived_from<Node> NodeType>
    ClassTypeBuilder& requiresNode()
        requires std::same_as<Root, Component>
    {
        return requiresNode(std::string(typeNameOf<NodeType>()));
    }

    template <std::derived_from<Component> Required>
    ClassTypeBuilder& requiresComponent()
        requires std::same_as<Root, Component>
    {
        return requiresComponent(std::string(typeNameOf<Required>()));
    }
};

} // namespace yqlora
