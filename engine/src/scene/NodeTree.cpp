#include "yqlora/scene/NodeTree.hpp"

#include "yqlora/scene/Node.hpp"

#include <utility>

namespace yqlora {

const Node* findNode(const Node& root, NodeId id)
{
    if (root.id() == id) {
        return &root;
    }
    for (const auto& child : root.children()) {
        if (const Node* found = findNode(*child, id)) {
            return found;
        }
    }

    return nullptr;
}

Node* findNode(Node& root, NodeId id)
{
    return const_cast<Node*>(findNode(std::as_const(root), id));
}

bool isInBranch(const Node& branch, const Node& node)
{
    for (const Node* n = &node; n; n = n->parent()) {
        if (n == &branch) {
            return true;
        }
    }

    return false;
}

std::optional<std::size_t> componentIndex(const Node& node, const Component& component)
{
    const auto components = node.components();
    for (std::size_t i = 0; i < components.size(); ++i) {
        if (components[i].get() == &component) {
            return i;
        }
    }

    return std::nullopt;
}

Component* componentAt(const Node& node, std::size_t index)
{
    return index < node.components().size() ? node.components()[index].get() : nullptr;
}

} // namespace yqlora
