#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/NodeId.hpp"

#include <cstddef>
#include <optional>
#include <string_view>

namespace yqlora {

class Component;
class Node;

// Поиск по дереву узлов. Свободные функции: им хватает открытого API узла.

// Узел с этим ID в ветке root (вместе с самим root); nullptr, если нет.
YQLORA_API Node* findNode(Node& root, NodeId id);
YQLORA_API const Node* findNode(const Node& root, NodeId id);

// Первый узел ветки с таким именем, в глубину, начиная с самого root.
// nullptr, если такого нет.
YQLORA_API Node* findNodeByName(Node& root, std::string_view name);

// node - это branch или его потомок.
YQLORA_API bool isInBranch(const Node& branch, const Node& node);

// Место компонента в списке узла; nullopt, если компонент не этого узла.
YQLORA_API std::optional<std::size_t> componentIndex(const Node& node, const Component& component);

// nullptr, если index за концом списка.
YQLORA_API Component* componentAt(const Node& node, std::size_t index);

} // namespace yqlora
