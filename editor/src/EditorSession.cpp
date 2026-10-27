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

namespace yqlora::editor {

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

} // namespace

EditorSession::EditorSession(Scene& scene, const SceneSerializer& serializer)
    : m_editScene(scene)
    , m_serializer(serializer)
{
}

bool EditorSession::open(Project project)
{
    stop();
    m_project = std::move(project);
    m_sceneId = m_project->mainScene();
    m_selection.clear();
    m_history.clear();

    // Загрузка при ошибке оставляет сцену как была, поэтому сначала очищаем:
    // от прошлого проекта ничего не должно остаться.
    m_editScene.setRoot(nullptr);
    const auto path = m_project->mainScenePath();
    if (!path || !m_serializer.load(m_editScene, m_project->directory() / *path)) {
        log::warn("Main scene of '{}' is not loaded, starting with an empty scene", m_project->name());

        return false;
    }

    return true;
}

void EditorSession::close()
{
    stop();
    m_editScene.setRoot(nullptr);
    m_project.reset();
    m_sceneId = 0;
    m_selection.clear();
    m_history.clear();
}

std::filesystem::path EditorSession::scenePath() const
{
    return m_project ? m_project->resources().path(m_sceneId).value_or(std::filesystem::path())
                     : std::filesystem::path();
}

bool EditorSession::openScene(const std::filesystem::path& path)
{
    stop();
    const ResourceId id = m_project ? m_project->resources().id(path) : 0;
    if (id == 0 || !m_serializer.load(m_editScene, m_project->directory() / path)) {
        return false;
    }
    m_sceneId = id;
    m_selection.clear();
    m_history.clear();

    return true;
}

void EditorSession::play()
{
    if (playing()) {
        return;
    }
    m_history.finishEdit();
    // Копия через формат файла: в ней те же ID узлов, и выделение остаётся на месте.
    auto copy = std::make_unique<Scene>();
    if (!m_serializer.fromJson(*copy, m_serializer.toJson(m_editScene))) {
        log::error("Cannot copy the scene for Play");

        return;
    }
    m_selectedBeforePlay = m_selection.selected();
    m_playScene = std::move(copy);
    m_paused = false;
}

void EditorSession::setPaused(bool paused)
{
    m_paused = playing() && paused;
}

void EditorSession::stop()
{
    if (!playing()) {
        return;
    }
    const NodeId selected = m_selection.selected();
    m_playScene->stop();
    m_playScene.reset();
    m_paused = false;
    // Узла, созданного в игре, в редактируемой сцене нет: возвращаем прежнее выделение.
    m_selection.select(findNode(selected) ? selected : m_selectedBeforePlay);
}

void EditorSession::update(float deltaSeconds)
{
    if (running()) {
        m_playScene->update(deltaSeconds);
    }
}

bool EditorSession::setMainScene(const std::filesystem::path& path)
{
    const ResourceId id = m_project ? m_project->resources().id(path) : 0;
    if (id == 0) {
        return false;
    }
    const ResourceId previous = m_project->mainScene();
    m_project->setMainScene(id);
    if (!m_project->save()) {
        m_project->setMainScene(previous);

        return false;
    }

    return true;
}

bool EditorSession::saveScene()
{
    // Во время игры в сцене состояние игры, а не то, что правили.
    if (!m_project || playing()) {
        return false;
    }

    const std::filesystem::path path = scenePath();
    if (path.empty() || !m_serializer.save(m_editScene, m_project->directory() / path)) {
        return false;
    }
    m_history.markSaved();

    return true;
}

bool EditorSession::execute(std::unique_ptr<Command> command)
{
    if (playing()) {
        return command && command->apply(*this);
    }

    return m_history.execute(*this, std::move(command));
}

bool EditorSession::undo()
{
    return !playing() && m_history.undo(*this);
}

bool EditorSession::redo()
{
    return !playing() && m_history.redo(*this);
}

Node* EditorSession::findNode(NodeId id)
{
    Scene& current = scene();

    return current.root() && id != 0 ? find(*current.root(), id) : nullptr;
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

} // namespace yqlora::editor
