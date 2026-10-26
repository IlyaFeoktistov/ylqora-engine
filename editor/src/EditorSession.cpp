#include "yqlora/editor/EditorSession.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <string>

namespace yqlora {

namespace {

Node* find(Node& node, NodeId id)
{
    if (node.id() == id) {
        return &node;
    }
    for (const auto& child : node.children()) {
        if (Node* found = find(*child, id)) {
            return found;
        }
    }

    return nullptr;
}

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

} // namespace

EditorSession::EditorSession(Scene& scene, const SceneSerializer& serializer)
    : m_scene(scene)
    , m_serializer(serializer)
{
}

bool EditorSession::open(const Project& project)
{
    m_project = project;
    m_selection.clear();
    m_modified = false;

    // Загрузка при ошибке оставляет сцену как была, поэтому сначала очищаем:
    // от прошлого проекта ничего не должно остаться.
    m_scene.setRoot(nullptr);
    if (!m_serializer.load(m_scene, project.mainScenePath())) {
        log::warn("Main scene of '{}' is not loaded, starting with an empty scene", project.name);

        return false;
    }

    return true;
}

void EditorSession::close()
{
    m_scene.setRoot(nullptr);
    m_project.reset();
    m_selection.clear();
    m_modified = false;
}

bool EditorSession::saveScene()
{
    if (!m_project) {
        return false;
    }

    if (!m_serializer.save(m_scene, m_project->mainScenePath())) {
        return false;
    }
    m_modified = false;

    return true;
}

Node* EditorSession::findNode(NodeId id)
{
    return m_scene.root() && id != 0 ? find(*m_scene.root(), id) : nullptr;
}

Node* EditorSession::selectedNode()
{
    return findNode(m_selection.selected());
}

Node* EditorSession::createNode(const TypeInfo& type, Node* parent)
{
    std::unique_ptr<Node> node = type.kind() == TypeKind::Node ? type.createNode() : nullptr;
    if (!node) {
        return nullptr;
    }
    Node* created = node.get();
    if (!m_scene.root()) {
        m_scene.setRoot(std::move(node));
    } else {
        Node& target = parent ? *parent : *m_scene.root();
        node->setName(uniqueChildName(target, type.name()));
        target.addChild(std::move(node));
    }
    m_selection.select(created->id());
    m_modified = true;

    return created;
}

bool EditorSession::deleteNode(NodeId id)
{
    Node* node = findNode(id);
    if (!node) {
        return false;
    }
    if (contains(*node, m_scene.activeCamera())) {
        m_scene.setActiveCamera(nullptr);
    }
    if (contains(*node, selectedNode())) {
        m_selection.clear();
    }
    if (node == m_scene.root()) {
        m_scene.setRoot(nullptr);
    } else {
        // Владение возвращается сюда и узел уничтожается вместе с детьми.
        node->parent()->removeChild(*node);
    }
    m_modified = true;

    return true;
}

Component* EditorSession::addComponent(Node& node, const TypeInfo& type)
{
    std::unique_ptr<Component> component = type.kind() == TypeKind::Component ? type.createComponent() : nullptr;
    if (!component) {
        return nullptr;
    }
    m_modified = true;

    return &node.addComponent(std::move(component));
}

bool EditorSession::removeComponent(Node& node, Component& component)
{
    if (!node.removeComponent(component)) {
        return false;
    }
    m_modified = true;

    return true;
}

} // namespace yqlora
