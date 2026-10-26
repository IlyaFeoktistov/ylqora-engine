#include "yqlora/scene/Node.hpp"

#include <algorithm>
#include <cassert>
#include <random>

namespace yqlora {

namespace {

NodeId generateId()
{
    // 64 случайных бита: совпадение ID у узлов из разных сцен практически исключено.
    thread_local std::mt19937_64 engine{std::random_device{}()};
    NodeId id = 0;
    while (id == 0) {
        id = engine();
    }

    return id;
}

} // namespace

Node::Node(std::string name)
    : m_id(generateId())
    , m_name(std::move(name))
{
}

Node::~Node() = default;

void Node::setId(NodeId id)
{
    assert(id != 0 && "setId: 0 means no node");
    m_id = id;
}

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
    const auto it = std::ranges::find_if(m_children, [&](const auto& c) {
        return c.get() == &child;
    });
    if (it == m_children.end()) {
        return nullptr;
    }
    std::unique_ptr<Node> result = std::move(*it);
    m_children.erase(it);
    result->m_parent = nullptr;
    result->onParentChanged();

    return result;
}

Component& Node::addComponent(std::unique_ptr<Component> component)
{
    assert(component && "addComponent: component is null");
    assert(!component->m_owner && "addComponent: component already has an owner");

    component->m_owner = this;
    Component& ref = *component;
    m_components.push_back(std::move(component));

    return ref;
}

Component& Node::addBuiltinComponent(std::unique_ptr<Component> component)
{
    assert(component && "addBuiltinComponent: component is null");
    component->m_builtin = true;

    return addComponent(std::move(component));
}

bool Node::canRemoveComponent(const Component& component) const
{
    return component.m_owner == this && !component.m_builtin;
}

std::unique_ptr<Component> Node::removeComponent(Component& component)
{
    if (!canRemoveComponent(component)) {
        return nullptr;
    }
    const auto it = std::ranges::find_if(m_components, [&](const auto& c) {
        return c.get() == &component;
    });
    if (it == m_components.end()) {
        return nullptr;
    }
    std::unique_ptr<Component> result = std::move(*it);
    m_components.erase(it);
    result->m_owner = nullptr;

    return result;
}

Node* Node::findChild(std::string_view name) const
{
    const auto it = std::ranges::find_if(m_children, [&](const auto& c) {
        return c->name() == name;
    });

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
