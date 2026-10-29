#pragma once

#include "yqlora/core/Export.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace yqlora {

class Component;
class Node;
class TypeInfo;
class TypeRegistry;

// Правила компонентов из реестра (requiresNode, requiresComponent, allowMultiple),
// см. BasicTypeBuilder. Правила базового типа действуют и на наследников.

// type - это тип base или его наследник.
YQLORA_API bool derivesFrom(const TypeRegistry& registry, const TypeInfo& type, std::string_view base);

// Компонент типа type можно подключить к node: узел подходит по requiresNode,
// и компонента ровно этого типа на нём ещё нет, если тип не allowMultiple.
// Обязательные компоненты здесь не проверяются, см. missingRequirements.
YQLORA_API bool canAttach(const TypeRegistry& registry, const TypeInfo& type, const Node& node);

// Каких обязательных компонентов не хватает на node, чтобы подключить type,
// вместе с обязательными для них самих. Порядок - в каком добавлять: сначала
// те, от которых зависят. nullopt, если какой-то из них не зарегистрирован,
// не создаётся или не подходит узлу.
YQLORA_API std::optional<std::vector<const TypeInfo*>> missingRequirements(
    const TypeRegistry& registry,
    const TypeInfo& type,
    const Node& node
);

// Компоненты node, которые останутся без обязательного, если снять component.
YQLORA_API std::vector<const Component*> dependentsOf(
    const TypeRegistry& registry,
    const Node& node,
    const Component& component
);

// Компонент можно снять с узла: он не встроенный и не нужен другим (dependentsOf).
YQLORA_API bool canDetach(const TypeRegistry& registry, const Node& node, const Component& component);

// Типы компонентов, которые можно добавить к node вместе с недостающими
// обязательными, в порядке регистрации.
YQLORA_API std::vector<const TypeInfo*> attachableTypes(const TypeRegistry& registry, const Node& node);

} // namespace yqlora
