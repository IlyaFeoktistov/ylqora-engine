#include "yqlora/editor/EditorSession.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <algorithm>
#include <optional>
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

// path лежит внутри from (или равен ему): тот же путь, но от to. Иначе nullopt.
std::optional<std::filesystem::path> replacePrefix(const std::filesystem::path& path,
                                                   const std::filesystem::path& from,
                                                   const std::filesystem::path& to)
{
    const auto [fromEnd, pathIt] = std::ranges::mismatch(from, path);
    if (path.empty() || fromEnd != from.end()) {
        return std::nullopt;
    }
    std::filesystem::path result = to;
    for (auto it = pathIt; it != path.end(); ++it) {
        result /= *it;
    }

    return result;
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
    stop();
    m_project = project;
    m_scenePath = project.mainScene;
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
    stop();
    m_scene.setRoot(nullptr);
    m_project.reset();
    m_scenePath.clear();
    m_selection.clear();
    m_modified = false;
}

bool EditorSession::openScene(const std::filesystem::path& path)
{
    stop();
    if (!m_project || !m_serializer.load(m_scene, m_project->directory / path)) {
        return false;
    }
    m_scenePath = path;
    m_selection.clear();
    m_modified = false;

    return true;
}

void EditorSession::play()
{
    if (m_playing) {
        return;
    }
    m_snapshot = m_serializer.toJson(m_scene);
    m_modifiedBeforePlay = m_modified;
    m_selectedBeforePlay = m_selection.selected();
    m_playing = true;
    m_paused = false;
}

void EditorSession::setPaused(bool paused)
{
    m_paused = m_playing && paused;
}

void EditorSession::stop()
{
    if (!m_playing) {
        return;
    }
    m_scene.stop();
    m_playing = false;
    m_paused = false;

    const NodeId selected = m_selection.selected();
    if (!m_serializer.fromJson(m_scene, m_snapshot)) {
        log::error("Cannot restore the scene after Play");
    }
    m_snapshot.clear();
    m_modified = m_modifiedBeforePlay;
    // ID узлов из файла сцены те же, поэтому выделение остаётся на том же узле,
    // если он был до Play. Узла, созданного в игре, больше нет: возвращаем прежнее выделение.
    m_selection.select(findNode(selected) ? selected : m_selectedBeforePlay);
}

bool EditorSession::setMainScene(const std::filesystem::path& path)
{
    if (!m_project) {
        return false;
    }
    const std::filesystem::path previous = m_project->mainScene;
    m_project->mainScene = path;
    if (!m_project->save()) {
        m_project->mainScene = previous;

        return false;
    }

    return true;
}

void EditorSession::fileRenamed(const std::filesystem::path& from, const std::filesystem::path& to)
{
    if (!m_project) {
        return;
    }
    // Переименовали саму сцену или папку, где она лежит.
    if (const auto scene = replacePrefix(m_scenePath, from, to)) {
        m_scenePath = *scene;
    }
    if (const auto main = replacePrefix(m_project->mainScene, from, to)) {
        setMainScene(*main);
    }
}

bool EditorSession::saveScene()
{
    // Во время игры в сцене состояние игры, а не то, что правили.
    if (!m_project || m_playing) {
        return false;
    }

    if (!m_serializer.save(m_scene, m_project->directory / m_scenePath)) {
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
    markModified();

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
    markModified();

    return true;
}

Component* EditorSession::addComponent(Node& node, const TypeInfo& type)
{
    std::unique_ptr<Component> component = type.kind() == TypeKind::Component ? type.createComponent() : nullptr;
    if (!component) {
        return nullptr;
    }
    markModified();

    return &node.addComponent(std::move(component));
}

bool EditorSession::removeComponent(Node& node, Component& component)
{
    if (!node.removeComponent(component)) {
        return false;
    }
    markModified();

    return true;
}

} // namespace yqlora
