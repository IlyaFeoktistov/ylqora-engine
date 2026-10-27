#include "yqlora/editor/SceneCommands.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Node3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace yqlora {

namespace {

bool contains(const Node& node, const Node* target)
{
    if (&node == target) {
        return true;
    }
    for (const auto& child : node.children()) {
        if (contains(*child, target)) {
            return true;
        }
    }

    return false;
}

// "Cube", "Cube2", "Cube3"... - первое свободное среди детей parent.
std::string uniqueChildName(const Node& parent, const std::string& base)
{
    if (!parent.findChild(base)) {
        return base;
    }
    for (int i = 2;; ++i) {
        std::string candidate = base + std::to_string(i);
        if (!parent.findChild(candidate)) {
            return candidate;
        }
    }
}

// Активная камера и выделение внутри ветки сбрасываются, чтобы не остались висячими.
DetachedBranch detach(EditorSession& session, Node& node)
{
    Scene& scene = session.scene();
    DetachedBranch branch;
    if (contains(node, scene.activeCamera())) {
        branch.activeCamera = scene.activeCamera();
        scene.setActiveCamera(nullptr);
    }
    if (contains(node, session.selectedNode())) {
        session.selection().clear();
    }

    Node* parent = node.parent();
    if (!parent) {
        branch.node = scene.takeRoot();
        // removeChild останавливает узел сам, а корень останавливаем мы.
        branch.node->stop();

        return branch;
    }
    const auto children = parent->children();
    const auto it = std::ranges::find_if(children, [&](const auto& child) {
        return child.get() == &node;
    });
    branch.parent = parent->id();
    branch.index = static_cast<std::size_t>(it - children.begin());
    branch.node = parent->removeChild(node);

    return branch;
}

// nullptr, если места больше нет.
Node* attach(EditorSession& session, DetachedBranch& branch)
{
    Scene& scene = session.scene();
    Node* node = branch.node.get();
    if (branch.parent == 0) {
        if (scene.root()) {
            return nullptr;
        }
        scene.setRoot(std::move(branch.node));
    } else {
        Node* parent = session.findNode(branch.parent);
        if (!parent) {
            return nullptr;
        }
        parent->insertChild(std::min(branch.index, parent->children().size()), std::move(branch.node));
    }
    if (branch.activeCamera) {
        scene.setActiveCamera(std::exchange(branch.activeCamera, nullptr));
    }

    return node;
}

Component* componentAt(Node& node, std::size_t index)
{
    return index < node.components().size() ? node.components()[index].get() : nullptr;
}

bool getProperty(EditorSession& session, const PropertyTarget& target, const Property& property, PropertyValue& value)
{
    Node* node = session.findNode(target.node);
    if (!node) {
        return false;
    }
    if (!target.component) {
        value = property.get(*node);

        return true;
    }
    const Component* component = componentAt(*node, *target.component);
    if (!component) {
        return false;
    }
    value = property.get(*component);

    return true;
}

bool setProperty(
    EditorSession& session,
    const PropertyTarget& target,
    const Property& property,
    const PropertyValue& value
)
{
    Node* node = session.findNode(target.node);
    if (!node) {
        return false;
    }
    if (!target.component) {
        return property.set(*node, value);
    }
    Component* component = componentAt(*node, *target.component);

    return component && property.set(*component, value);
}

} // namespace

CreateNodeCommand::CreateNodeCommand(const TypeInfo& type, NodeId parent)
    : m_type(type)
    , m_parent(parent)
{
    if (type.kind() == TypeKind::Node) {
        m_branch.node = type.createNode();
    }
    if (m_branch.node) {
        m_created = m_branch.node->id();
    }
}

bool CreateNodeCommand::apply(EditorSession& session)
{
    if (!m_branch.node) {
        return false;
    }
    Scene& scene = session.scene();
    if (!m_placed && scene.root()) {
        Node* target = m_parent != 0 ? session.findNode(m_parent) : scene.root();
        if (!target) {
            return false;
        }
        m_branch.node->setName(uniqueChildName(*target, m_type.name()));
        m_branch.parent = target->id();
        m_branch.index = target->children().size();
    }
    // Повтор после отмены вернёт узел на то же место.
    m_placed = true;

    const NodeId selected = session.selection().selected();
    if (!attach(session, m_branch)) {
        return false;
    }
    m_previousSelection = selected;
    session.selection().select(m_created);

    return true;
}

void CreateNodeCommand::revert(EditorSession& session)
{
    if (Node* node = session.findNode(m_created)) {
        m_branch = detach(session, *node);
    }
    session.selection().select(m_previousSelection);
}

DeleteNodeCommand::DeleteNodeCommand(NodeId node)
    : m_node(node)
{
}

bool DeleteNodeCommand::apply(EditorSession& session)
{
    Node* node = session.findNode(m_node);
    if (!node) {
        return false;
    }
    m_previousSelection = session.selection().selected();
    m_branch = detach(session, *node);

    return true;
}

void DeleteNodeCommand::revert(EditorSession& session)
{
    if (attach(session, m_branch)) {
        session.selection().select(m_previousSelection);
    }
}

AddComponentCommand::AddComponentCommand(NodeId node, const TypeInfo& type)
    : m_node(node)
    , m_type(type)
{
}

bool AddComponentCommand::apply(EditorSession& session)
{
    Node* node = session.findNode(m_node);
    if (!node) {
        return false;
    }
    if (m_detached) {
        node->insertComponent(std::min(*m_index, node->components().size()), std::move(m_detached));

        return true;
    }

    std::unique_ptr<Component> component = m_type.kind() == TypeKind::Component ? m_type.createComponent() : nullptr;
    if (!component) {
        return false;
    }
    m_index = node->components().size();
    node->addComponent(std::move(component));

    return true;
}

void AddComponentCommand::revert(EditorSession& session)
{
    Node* node = session.findNode(m_node);
    Component* component = node && m_index ? componentAt(*node, *m_index) : nullptr;
    if (component) {
        m_detached = node->removeComponent(*component);
    }
}

RemoveComponentCommand::RemoveComponentCommand(NodeId node, std::size_t index)
    : m_node(node)
    , m_index(index)
{
}

bool RemoveComponentCommand::apply(EditorSession& session)
{
    Node* node = session.findNode(m_node);
    Component* component = node ? componentAt(*node, m_index) : nullptr;
    if (!component) {
        return false;
    }
    m_detached = node->removeComponent(*component);

    return m_detached != nullptr;
}

void RemoveComponentCommand::revert(EditorSession& session)
{
    Node* node = session.findNode(m_node);
    if (node && m_detached) {
        node->insertComponent(std::min(m_index, node->components().size()), std::move(m_detached));
    }
}

SetPropertyCommand::SetPropertyCommand(PropertyTarget target, const Property& property, PropertyValue value)
    : m_target(target)
    , m_property(property)
    , m_value(std::move(value))
{
}

bool SetPropertyCommand::apply(EditorSession& session)
{
    if (!m_previous) {
        PropertyValue previous;
        if (!getProperty(session, m_target, m_property, previous)) {
            return false;
        }
        m_previous = std::move(previous);
    }

    return setProperty(session, m_target, m_property, m_value);
}

void SetPropertyCommand::revert(EditorSession& session)
{
    if (m_previous) {
        setProperty(session, m_target, m_property, *m_previous);
    }
}

bool SetPropertyCommand::merge(const Command& next)
{
    const auto* other = dynamic_cast<const SetPropertyCommand*>(&next);
    if (!other || other->m_target != m_target || &other->m_property != &m_property) {
        return false;
    }
    m_value = other->m_value;

    return true;
}

SetTransformCommand::SetTransformCommand(NodeId node, const TransformValues& values)
    : m_node(node)
    , m_values(values)
{
}

bool SetTransformCommand::apply(EditorSession& session)
{
    auto* node = dynamic_cast<Node3D*>(session.findNode(m_node));
    if (!node) {
        return false;
    }
    if (!m_previous) {
        m_previous = transformValues(node->transform());
    }
    setTransformValues(node->transform(), m_values);

    return true;
}

void SetTransformCommand::revert(EditorSession& session)
{
    auto* node = dynamic_cast<Node3D*>(session.findNode(m_node));
    if (node && m_previous) {
        setTransformValues(node->transform(), *m_previous);
    }
}

bool SetTransformCommand::merge(const Command& next)
{
    const auto* other = dynamic_cast<const SetTransformCommand*>(&next);
    if (!other || other->m_node != m_node) {
        return false;
    }
    m_values = other->m_values;

    return true;
}

} // namespace yqlora
