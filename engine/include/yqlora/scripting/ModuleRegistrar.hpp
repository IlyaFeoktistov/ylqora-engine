#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scripting/Script.hpp"

#include <concepts>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace yqlora {

class ModuleRegistrar;

// Входная функция модуля игры: единственное место, где библиотека
// регистрирует свои типы. Глобальных регистраторов нет, поэтому порядок
// статической инициализации ни на что не влияет.
//
//   YQLORA_MODULE_ENTRY(registrar)
//   {
//       registrar.registerScript<Player>();
//   }
#define YQLORA_MODULE_ENTRY(registrar)                                                                                 \
    extern "C" YQLORA_MODULE_EXPORT void yqloraModuleEntry(::yqlora::ModuleRegistrar& registrar)

using ModuleEntry = void (*)(ModuleRegistrar&);

// Регистрация, которую реестр не принял: имя уже занято типом модуля takenBy.
struct RejectedType {
    std::string name;
    std::string takenBy;
};

// Правила типа скрипта сверх его полей (поля - в YQLORA_FIELDS). У отклонённого
// скрипта цепочка ничего не делает.
template <std::derived_from<Script> T>
class ScriptBuilder {
public:
    ScriptBuilder() = default;

    explicit ScriptBuilder(ClassTypeBuilder<T, Component> builder)
        : m_builder(builder)
    {
    }

    // Класс переименован: файлы со старым именем типа читаются, пишется новое.
    ScriptBuilder& formerly(std::string oldName)
    {
        if (m_builder) {
            m_builder->formerTypeName(std::move(oldName));
        }

        return *this;
    }

    template <std::derived_from<Node> NodeType>
    ScriptBuilder& requiresNode()
    {
        if (m_builder) {
            m_builder->template requiresNode<NodeType>();
        }

        return *this;
    }

    template <std::derived_from<Component> Required>
    ScriptBuilder& requiresComponent()
    {
        if (m_builder) {
            m_builder->template requiresComponent<Required>();
        }

        return *this;
    }

    ScriptBuilder& executionOrder(int order)
    {
        if (m_builder) {
            m_builder->executionOrder(order);
        }

        return *this;
    }

private:
    std::optional<ClassTypeBuilder<T, Component>> m_builder;
};

// То, что входная функция модуля получает от движка: регистрация типов на
// модуль module. Имя, уже занятое любым типом (своего или чужого модуля, в
// том числе прежнее имя), не регистрируется, а попадает в rejected(), чтобы
// загрузивший модуль сообщил об этом, не роняя редактор.
class YQLORA_API ModuleRegistrar {
public:
    ModuleRegistrar(TypeRegistry& registry, std::string module);

    ModuleRegistrar(const ModuleRegistrar&) = delete;
    ModuleRegistrar& operator=(const ModuleRegistrar&) = delete;

    // Скрипт на C++: тип с пометкой "скрипт", несколько на узле, и его поля из
    // YQLORA_FIELDS со всеми атрибутами.
    template <std::derived_from<Script> T>
    ScriptBuilder<T> registerScript()
    {
        if (!accept(typeNameOf<T>())) {
            return ScriptBuilder<T>();
        }
        auto builder = m_registry.registerComponent<T>();
        builder.script().allowMultiple();
        std::apply(
            [&](const auto&... fields) {
                (addField(builder, fields), ...);
            },
            ScriptFields::of<T>()
        );

        return ScriptBuilder<T>(builder);
    }

    std::span<const RejectedType> rejected() const
    {
        return m_rejected;
    }

private:
    // false и запись в rejected, если имя занято.
    bool accept(std::string_view name);

    template <typename Builder, typename Field>
    static void addField(Builder& builder, const Field& field)
    {
        builder.property(std::string(field.name), field.member, field.hint);
        if (!field.enumNames.empty()) {
            builder.enumNames(std::vector<std::string_view>(field.enumNames.begin(), field.enumNames.end()));
        }
        builder.withAttributes(field.attributes);
    }

    TypeRegistry& m_registry;
    TypeRegistry::ModuleScope m_scope;
    std::vector<RejectedType> m_rejected;
};

} // namespace yqlora
