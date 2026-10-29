#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/SceneSystem.hpp"

#include <cassert>
#include <concepts>
#include <functional>
#include <initializer_list>
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
    System,
};

// Описание типа узла или компонента: по нему редактор строит меню создания
// и инспектор, а файлы сцен создают объекты по имени типа.
class YQLORA_API TypeInfo {
public:
    const std::string& name() const
    {
        return m_name;
    }

    // Пусто у Node, Component и SceneSystem: это корни иерархий.
    const std::string& baseName() const
    {
        return m_baseName;
    }

    TypeKind kind() const
    {
        return m_kind;
    }

    // Модуль, который зарегистрировал тип: движок или dll игры. Типы модуля
    // убираются из реестра вместе, когда dll выгружают.
    const std::string& module() const
    {
        return m_module;
    }

    // Встроенные компоненты (Transform) создаёт сам узел, поэтому они не создаваемые.
    bool creatable() const
    {
        return m_createNode != nullptr || m_createComponent != nullptr || m_createSystem != nullptr;
    }

    // Встроенный компонент: его создаёт сам узел, снять и выключить его нельзя.
    // Это несоздаваемые компоненты, кроме корня Component.
    bool builtin() const
    {
        return m_kind == TypeKind::Component && !creatable() && !m_baseName.empty();
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

    // Правила компонента, только свои, без базовых. Проверяют их функции
    // из ComponentRules.hpp, они же учитывают правила базовых типов.

    // Тип узла, на котором может жить компонент; пусто - на любом.
    const std::string& requiredNode() const
    {
        return m_requiredNode;
    }

    // Компоненты, которые должны быть на том же узле.
    std::span<const std::string> requiredComponents() const
    {
        return m_requiredComponents;
    }

    // Можно несколько компонентов этого типа на одном узле.
    bool allowsMultiple() const
    {
        return m_allowMultiple;
    }

    // Фазы кадра, которые переопределил тип компонента (см. LifecyclePhase.hpp).
    LifecyclePhases phases() const
    {
        return m_phases;
    }

    // Порядок выполнения по умолчанию из кода. Действующий порядок - executionOrderOf.
    int executionOrder() const
    {
        return m_executionOrder;
    }

    // nullptr, если это не узел (не компонент) или тип нельзя создать.
    std::unique_ptr<Node> createNode() const;
    std::unique_ptr<Component> createComponent() const;
    std::unique_ptr<SceneSystem> createSystem(Scene& scene) const;

private:
    friend class TypeRegistry;
    template <typename Class, typename Root>
    friend class TypeBuilder;

    std::string m_name;
    std::string m_baseName;
    std::string m_module;
    TypeKind m_kind = TypeKind::Node;
    std::function<std::unique_ptr<Node>()> m_createNode;
    std::function<std::unique_ptr<Component>()> m_createComponent;
    std::function<std::unique_ptr<SceneSystem>(Scene&)> m_createSystem;
    std::vector<Property> m_properties;
    std::string m_requiredNode;
    std::vector<std::string> m_requiredComponents;
    bool m_allowMultiple = false;
    LifecyclePhases m_phases = 0;
    int m_executionOrder = 0;
};

// Добавляет свойства к только что зарегистрированному типу:
//   registry.registerNode<Camera3D>()
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
        requires(!std::same_as<Root, SceneSystem>)
    {
        using Value = std::remove_cvref_t<std::invoke_result_t<Getter, const Class&>>;

        Property property;
        property.m_name = std::move(name);
        property.m_type = propertyTypeOf<Value>();
        property.m_hint = hint;
        property.m_forNode = std::is_same_v<Root, Node>;
        using Storage = PropertyStorage<Value>;
        using Stored = typename Storage::Type;
        property.m_get = [getter](const void* object) -> PropertyValue {
            return PropertyValue(std::in_place_type<Stored>, Storage::store(std::invoke(getter, cast(object))));
        };
        property.m_set = [setter](void* object, const PropertyValue& value) {
            std::invoke(setter, cast(object), Storage::load(std::get<Stored>(value)));
        };
        m_info.m_properties.push_back(std::move(property));

        return *this;
    }

    // Имена значений перечисления, которое добавили последним .property():
    //   .property("projection", &Camera3D::projection, &Camera3D::setProjection)
    //   .enumNames({"Perspective", "Orthographic"})
    TypeBuilder& enumNames(std::initializer_list<std::string_view> names)
    {
        assert(
            !m_info.m_properties.empty() && m_info.m_properties.back().m_type == PropertyType::Enum
            && "enumNames: the last property is not an enum"
        );
        for (const std::string_view name : names) {
            m_info.m_properties.back().m_enumNames.emplace_back(name);
        }

        return *this;
    }

    // Компонент живёт только на узлах типа NodeType и его наследников:
    //   registry.registerComponent<MeshRenderer>().requiresNode<Node3D>();
    template <std::derived_from<Node> NodeType>
    TypeBuilder& requiresNode()
        requires std::same_as<Root, Component>
    {
        m_info.m_requiredNode = std::string(typeNameOf<NodeType>());

        return *this;
    }

    // На том же узле нужен компонент типа Required (или его наследника).
    // Редактор добавляет его сам, а снять его нельзя, пока от него зависят.
    template <std::derived_from<Component> Required>
    TypeBuilder& requiresComponent()
        requires std::same_as<Root, Component>
    {
        m_info.m_requiredComponents.emplace_back(typeNameOf<Required>());

        return *this;
    }

    // Порядок выполнения в фазах кадра по умолчанию (меньше - раньше, по
    // умолчанию 0). Настройки проекта могут его переопределить, см. executionOrderOf.
    TypeBuilder& executionOrder(int order)
        requires std::same_as<Root, Component>
    {
        m_info.m_executionOrder = order;

        return *this;
    }

    // Несколько компонентов этого типа на одном узле. По умолчанию один.
    TypeBuilder& allowMultiple()
        requires std::same_as<Root, Component>
    {
        m_info.m_allowMultiple = true;

        return *this;
    }

    // Свойство-поле, когда getter и setter не нужны: .property("radius", &SphereShape::radius);
    template <typename Value>
    TypeBuilder& property(std::string name, Value Class::* field, PropertyHint hint = PropertyHint::None)
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
// registerEngineTypes(), игра регистрирует свои в своём модуле (см. ModuleScope).
class YQLORA_API TypeRegistry {
public:
    // Модуль типов самого движка. Функция, а не константа: см. YQLORA_TYPE.
    static constexpr std::string_view engineModule()
    {
        return "engine";
    }

    // Пока объект жив, новые типы записываются на модуль module:
    //   TypeRegistry::ModuleScope scope(registry, "game");
    //   registry.registerComponent<Player>();
    class ModuleScope {
    public:
        ModuleScope(TypeRegistry& registry, std::string module)
            : m_registry(registry)
            , m_previous(std::exchange(registry.m_module, std::move(module)))
        {
        }

        ~ModuleScope()
        {
            m_registry.m_module = std::move(m_previous);
        }

        ModuleScope(const ModuleScope&) = delete;
        ModuleScope& operator=(const ModuleScope&) = delete;

    private:
        TypeRegistry& m_registry;
        std::string m_previous;
    };

    TypeRegistry() = default;

    TypeRegistry(const TypeRegistry&) = delete;
    TypeRegistry& operator=(const TypeRegistry&) = delete;

    // Имя типа и база берутся из YQLORA_TYPE в классе.
    template <std::derived_from<Node> T>
    TypeBuilder<T, Node> registerNode()
    {
        TypeInfo& info = add<T>(TypeKind::Node);
        if constexpr (std::is_default_constructible_v<T>) {
            info.m_createNode = [] {
                return std::unique_ptr<Node>(std::make_unique<T>());
            };
        }

        return TypeBuilder<T, Node>(info);
    }

    template <std::derived_from<Component> T>
    TypeBuilder<T, Component> registerComponent()
    {
        TypeInfo& info = add<T>(TypeKind::Component);
        info.m_phases = PhaseDetector::phasesOf<T>();
        if constexpr (std::is_default_constructible_v<T>) {
            info.m_createComponent = [] {
                return std::unique_ptr<Component>(std::make_unique<T>());
            };
        }

        return TypeBuilder<T, Component>(info);
    }

    // Система сцены (см. SceneSystem): каждая новая сцена создаёт её через
    // конструктор T(Scene&).
    template <std::derived_from<SceneSystem> T>
    TypeBuilder<T, SceneSystem> registerSystem()
    {
        TypeInfo& info = add<T>(TypeKind::System);
        if constexpr (std::is_constructible_v<T, Scene&> && !std::is_abstract_v<T>) {
            info.m_createSystem = [](Scene& scene) {
                return std::unique_ptr<SceneSystem>(std::make_unique<T>(scene));
            };
        }

        return TypeBuilder<T, SceneSystem>(info);
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

    // Тип и все его базовые, от самого типа к корню.
    std::vector<const TypeInfo*> chain(const TypeInfo& type) const;

    // Свойства типа вместе с базовыми, от корня (Node, Component) к наследнику.
    // У встроенного компонента нет свойств корня: выключить его нельзя.
    std::vector<const Property*> properties(const TypeInfo& type) const;

    // Убирает все типы модуля, например перед выгрузкой dll игры: иначе в
    // реестре остались бы функции из выгруженного кода. Объектов этих типов
    // к этому моменту быть не должно. Возвращает, сколько типов убрано.
    std::size_t removeModule(std::string_view module);

private:
    template <typename T>
    TypeInfo& add(TypeKind kind)
    {
        const std::string_view name = typeNameOf<T>();
        assert(!find(name) && "TypeRegistry: type name is already registered");
        assert(!find<T>() && "TypeRegistry: C++ type is already registered");

        auto info = std::make_unique<TypeInfo>();
        info->m_name = std::string(name);
        info->m_module = m_module;
        info->m_kind = kind;
        // У корней (Node, Component, SceneSystem) базы в реестре нет.
        using Base = typename T::YqloraBaseType;
        if constexpr (!std::is_void_v<Base>) {
            static_assert(std::derived_from<T, Base>, "YQLORA_TYPE: the base is not a base of the class");
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
    std::string m_module{engineModule()};
};

// Типы движка: Node, Node3D, Camera3D, Component, Transform3D, MeshRenderer,
// SceneSystem, TransformSystem3D.
YQLORA_API void registerEngineTypes(TypeRegistry& registry);

} // namespace yqlora
