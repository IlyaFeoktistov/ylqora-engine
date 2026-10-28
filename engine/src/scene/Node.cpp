#include "yqlora/scene/Node.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/ComponentRules.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Scene.hpp"

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
    return insertChild(m_children.size(), std::move(child));
}

Node& Node::insertChild(std::size_t index, std::unique_ptr<Node> child)
{
    assert(child && "insertChild: child is null");
    assert(!child->m_parent && "insertChild: node already has a parent, call removeChild first");
    assert(index <= m_children.size() && "insertChild: index is out of range");

    child->m_parent = this;
    Node& ref = *child;
    m_children.insert(m_children.begin() + static_cast<std::ptrdiff_t>(index), std::move(child));
    ref.onParentChanged();
    if (m_scene) {
        ref.enterScene(*m_scene);
    }

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
    // Узел уходит из дерева посреди игры: он и его дети останавливаются.
    child.stop();
    if (child.m_scene) {
        child.exitScene();
    }
    std::unique_ptr<Node> result = std::move(*it);
    m_children.erase(it);
    result->m_parent = nullptr;
    result->onParentChanged();

    return result;
}

Component& Node::addComponent(std::unique_ptr<Component> component)
{
    if (m_scene) {
        addRequiredComponents(*component);
    }

    return insertComponent(m_components.size(), std::move(component));
}

void Node::addRequiredComponents(const Component& component)
{
    const TypeRegistry& registry = m_scene->services().registry();
    const TypeInfo* type = registry.find(component.typeName());
    if (!type) {
        // Тип не зарегистрирован: его правил никто не знает.
        return;
    }
    const auto missing = canAttach(registry, *type, *this) ? missingRequirements(registry, *type, *this) : std::nullopt;
    if (!missing) {
        log::error("Component '{}' does not fit node '{}' of type '{}'", type->name(), name(), typeName());
        assert(false && "addComponent: the component rules are broken, see ComponentRules.hpp");

        return;
    }
    for (const TypeInfo* required : *missing) {
        insertComponent(m_components.size(), required->createComponent());
    }
}

Component& Node::insertComponent(std::size_t index, std::unique_ptr<Component> component)
{
    assert(component && "insertComponent: component is null");
    assert(!component->m_owner && "insertComponent: component already has an owner");
    assert(index <= m_components.size() && "insertComponent: index is out of range");

    component->m_owner = this;
    Component& ref = *component;
    m_components.insert(m_components.begin() + static_cast<std::ptrdiff_t>(index), std::move(component));
    if (m_scene) {
        m_scene->indexComponent(ref);
        ref.onEnterScene();
    }

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
    if (component.m_started) {
        component.m_started = false;
        component.onStop();
    }
    if (m_scene) {
        component.onExitScene();
        m_scene->unindexComponent(component);
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
    // По индексу, а не итератором: во время обновления компоненты и дети
    // могут добавляться, и вектор переедет в другую память.
    for (std::size_t i = 0; i < m_components.size(); ++i) {
        Component& component = *m_components[i];
        if (!component.m_started) {
            component.m_started = true;
            component.onStart();
        }
        component.onUpdate(deltaSeconds);
    }
    for (std::size_t i = 0; i < m_children.size(); ++i) {
        m_children[i]->update(deltaSeconds);
    }
}

void Node::enterScene(Scene& scene)
{
    m_scene = &scene;
    for (const auto& component : m_components) {
        scene.indexComponent(*component);
        component->onEnterScene();
    }
    onEnterScene();
    for (std::size_t i = 0; i < m_children.size(); ++i) {
        m_children[i]->enterScene(scene);
    }
}

void Node::exitScene()
{
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it) {
        (*it)->exitScene();
    }
    onExitScene();
    for (const auto& component : m_components) {
        component->onExitScene();
        m_scene->unindexComponent(*component);
    }
    m_scene = nullptr;
}

void Node::stop()
{
    for (const auto& child : m_children) {
        child->stop();
    }
    for (const auto& component : m_components) {
        if (component->m_started) {
            component->m_started = false;
            component->onStop();
        }
    }
}

} // namespace yqlora
