#include "yqlora/scene/Node.hpp"

#include <algorithm>
#include <cassert>

namespace yqlora {

Node::Node(std::string name)
    : m_name(std::move(name))
{
}

Node::~Node() = default;

Node& Node::addChild(std::unique_ptr<Node> child)
{
    assert(child && "addChild: child is null");
    assert(!child->m_parent && "addChild: node already has a parent, call removeChild first");

    child->m_parent = this;
    Node& ref = *child;
    m_children.push_back(std::move(child));
    ref.onParentChanged();
    return ref;
}

std::unique_ptr<Node> Node::removeChild(Node& child)
{
    const auto it = std::ranges::find_if(m_children, [&](const auto& c) { return c.get() == &child; });
    if (it == m_children.end()) {
        return nullptr;
    }
    std::unique_ptr<Node> result = std::move(*it);
    m_children.erase(it);
    result->m_parent = nullptr;
    result->onParentChanged();
    return result;
}

Node* Node::findChild(std::string_view name) const
{
    const auto it = std::ranges::find_if(m_children, [&](const auto& c) { return c->name() == name; });
    return it != m_children.end() ? it->get() : nullptr;
}

void Node::update(float deltaSeconds)
{
    onUpdate(deltaSeconds);
    for (const auto& child : m_children) {
        child->update(deltaSeconds);
    }
}

} // namespace yqlora
