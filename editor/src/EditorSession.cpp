#include "yqlora/editor/EditorSession.hpp"

#include "yqlora/editor/SceneCommands.hpp"

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

// path лежит внутри from (или равен ему): тот же путь, но от to. Иначе nullopt.
std::optional<std::filesystem::path> replacePrefix(
    const std::filesystem::path& path,
    const std::filesystem::path& from,
    const std::filesystem::path& to
)
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
    m_history.clear();

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
    m_history.clear();
}

bool EditorSession::openScene(const std::filesystem::path& path)
{
    stop();
    if (!m_project || !m_serializer.load(m_scene, m_project->directory / path)) {
        return false;
    }
    m_scenePath = path;
    m_selection.clear();
    m_history.clear();

    return true;
}

void EditorSession::play()
{
    if (m_playing) {
        return;
    }
    m_history.finishEdit();
    m_snapshot = m_serializer.toJson(m_scene);
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
    m_history.markSaved();

    return true;
}

bool EditorSession::execute(std::unique_ptr<Command> command)
{
    if (m_playing) {
        return command && command->apply(*this);
    }

    return m_history.execute(*this, std::move(command));
}

bool EditorSession::undo()
{
    return !m_playing && m_history.undo(*this);
}

bool EditorSession::redo()
{
    return !m_playing && m_history.redo(*this);
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
    auto command = std::make_unique<CreateNodeCommand>(type, parent ? parent->id() : 0);
    const NodeId created = command->created();
    if (created == 0 || !execute(std::move(command))) {
        return nullptr;
    }

    return findNode(created);
}

bool EditorSession::deleteNode(NodeId id)
{
    return execute(std::make_unique<DeleteNodeCommand>(id));
}

Component* EditorSession::addComponent(Node& node, const TypeInfo& type)
{
    const std::size_t index = node.components().size();
    if (!execute(std::make_unique<AddComponentCommand>(node.id(), type))) {
        return nullptr;
    }

    return node.components()[index].get();
}

bool EditorSession::removeComponent(Node& node, Component& component)
{
    const auto index = componentIndex(node, component);

    return index && execute(std::make_unique<RemoveComponentCommand>(node.id(), *index));
}

bool EditorSession::setProperty(Node& node, const Property& property, const PropertyValue& value)
{
    return execute(
        std::make_unique<SetPropertyCommand>(
            PropertyTarget{.node = node.id(), .component = std::nullopt},
            property,
            value
        )
    );
}

bool EditorSession::setProperty(Component& component, const Property& property, const PropertyValue& value)
{
    Node* node = component.owner();
    const auto index = node ? componentIndex(*node, component) : std::nullopt;
    if (!index) {
        return false;
    }

    return execute(
        std::make_unique<SetPropertyCommand>(PropertyTarget{.node = node->id(), .component = index}, property, value)
    );
}

bool EditorSession::setTransform(Node3D& node, const TransformValues& values)
{
    return execute(std::make_unique<SetTransformCommand>(node.id(), values));
}

} // namespace yqlora
