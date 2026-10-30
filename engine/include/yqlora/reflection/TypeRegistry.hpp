#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/reflection/TypeBuilder.hpp"
#include "yqlora/reflection/TypeInfo.hpp"
#include "yqlora/reflection/TypeName.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/LifecyclePhase.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/SceneSystem.hpp"

#include <cassert>
#include <concepts>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <vector>

namespace yqlora {

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

    // Тип, описанный во время выполнения, без своего C++-класса (см.
    // TypeBuilder). base - уже зарегистрированный тип того же вида; пусто
    // только у корней иерархий.
    TypeBuilder registerType(std::string name, TypeKind kind, std::string base);

    // Имя типа и база берутся из YQLORA_TYPE в классе.
    template <std::derived_from<Node> T>
    ClassTypeBuilder<T, Node> registerNode()
    {
        TypeInfo& info = add<T>(TypeKind::Node);
        if constexpr (std::is_default_constructible_v<T>) {
            info.m_createNode = [] {
                return std::unique_ptr<Node>(std::make_unique<T>());
            };
        }

        return ClassTypeBuilder<T, Node>(info);
    }

    template <std::derived_from<Component> T>
    ClassTypeBuilder<T, Component> registerComponent()
    {
        TypeInfo& info = add<T>(TypeKind::Component);
        info.m_phases = PhaseDetector::phasesOf<T>();
        if constexpr (std::is_default_constructible_v<T>) {
            info.m_createComponent = [] {
                return std::unique_ptr<Component>(std::make_unique<T>());
            };
        }

        return ClassTypeBuilder<T, Component>(info);
    }

    // Система сцены (см. SceneSystem): каждая новая сцена создаёт её через
    // конструктор T(Scene&).
    template <std::derived_from<SceneSystem> T>
    ClassTypeBuilder<T, SceneSystem> registerSystem()
    {
        TypeInfo& info = add<T>(TypeKind::System);
        if constexpr (std::is_constructible_v<T, Scene&> && !std::is_abstract_v<T>) {
            info.m_createSystem = [](Scene& scene) {
                return std::unique_ptr<SceneSystem>(std::make_unique<T>(scene));
            };
        }

        return ClassTypeBuilder<T, SceneSystem>(info);
    }

    // nullptr, если тип не зарегистрирован.
    const TypeInfo* find(std::string_view name) const;

    // Только типы - C++-классы; у типов, описанных во время выполнения, своего класса нет.
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

    // Методы типа вместе с базовыми, от корня к наследнику.
    std::vector<const Method*> methods(const TypeInfo& type) const;

    // Убирает все типы модуля, например перед выгрузкой dll игры: иначе в
    // реестре остались бы функции из выгруженного кода. Объектов этих типов
    // к этому моменту быть не должно. Возвращает, сколько типов убрано.
    std::size_t removeModule(std::string_view module);

private:
    template <typename T>
    TypeInfo& add(TypeKind kind)
    {
        assert(!find<T>() && "TypeRegistry: C++ type is already registered");
        // У корней (Node, Component, SceneSystem) базы в реестре нет.
        using Base = typename T::YqloraBaseType;
        std::string base;
        if constexpr (!std::is_void_v<Base>) {
            static_assert(std::derived_from<T, Base>, "YQLORA_TYPE: the base is not a base of the class");
            base = std::string(typeNameOf<Base>());
        }
        TypeInfo& info = addType(std::string(typeNameOf<T>()), kind, std::move(base));
        m_byCppType.emplace(std::type_index(typeid(T)), &info);

        return info;
    }

    // Общий путь любой регистрации: проверки имени и базы, модуль, списки.
    TypeInfo& addType(std::string name, TypeKind kind, std::string base);

    std::vector<std::unique_ptr<TypeInfo>> m_types;
    std::map<std::string, TypeInfo*, std::less<>> m_byName;
    std::map<std::type_index, TypeInfo*> m_byCppType;
    std::string m_module{engineModule()};
};

// Типы движка: Node, Node3D, Camera3D, Component, Transform3D, MeshRenderer,
// SceneSystem, TransformSystem3D, RenderSystem.
YQLORA_API void registerEngineTypes(TypeRegistry& registry);

} // namespace yqlora
