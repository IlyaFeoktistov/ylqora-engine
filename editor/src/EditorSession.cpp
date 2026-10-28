#include "yqlora/editor/EditorSession.hpp"

#include "yqlora/editor/SceneCommands.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/ComponentRules.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/NodeTree.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace yqlora::editor {

namespace {

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
    m_selectionBeforePlay = m_selection;
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
    const Selection selected = m_selection;
    m_playScene->stop();
    m_playScene.reset();
    m_paused = false;
    // Узлов, созданных в игре, в редактируемой сцене нет. Если не осталось
    // ни одного выбранного, возвращаем выделение, каким оно было до Play.
    m_selection.clear();
    for (const NodeId id : selected.nodes()) {
        if (findNode(id)) {
            m_selection.toggle(id);
        }
    }
    if (m_selection.empty()) {
        m_selection = m_selectionBeforePlay;
    }
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

    return current.root() && id != 0 ? yqlora::findNode(*current.root(), id) : nullptr;
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

bool EditorSession::deleteNodes(std::span<const NodeId> ids)
{
    const auto insideAnother = [&](NodeId id) {
        const Node* node = findNode(id);
        for (const Node* parent = node ? node->parent() : nullptr; parent; parent = parent->parent()) {
            if (std::ranges::find(ids, parent->id()) != ids.end()) {
                return true;
            }
        }

        return false;
    };

    std::vector<std::unique_ptr<Command>> commands;
    for (const NodeId id : ids) {
        if (!insideAnother(id)) {
            commands.push_back(std::make_unique<DeleteNodeCommand>(id));
        }
    }

    return execute(std::make_unique<CompositeCommand>(std::move(commands)));
}

Component* EditorSession::addComponent(Node& node, const TypeInfo& type)
{
    const TypeRegistry& registry = this->registry();
    const auto missing = canAttach(registry, type, node) ? missingRequirements(registry, type, node) : std::nullopt;
    if (!missing) {
        return nullptr;
    }

    std::vector<std::unique_ptr<Command>> commands;
    for (const TypeInfo* required : *missing) {
        commands.push_back(std::make_unique<AddComponentCommand>(node.id(), *required));
    }
    commands.push_back(std::make_unique<AddComponentCommand>(node.id(), type));
    if (!execute(std::make_unique<CompositeCommand>(std::move(commands)))) {
        return nullptr;
    }

    return node.components().back().get();
}

bool EditorSession::removeComponent(Node& node, Component& component)
{
    const auto index = componentIndex(node, component);

    return index && canRemoveComponent(node, component)
           && execute(std::make_unique<RemoveComponentCommand>(node.id(), *index));
}

bool EditorSession::canRemoveComponent(const Node& node, const Component& component) const
{
    return node.canRemoveComponent(component) && componentDependents(node, component).empty();
}

const TypeRegistry& EditorSession::registry() const
{
    return m_serializer.registry();
}

std::vector<const TypeInfo*> EditorSession::addableComponents(const Node& node) const
{
    const TypeRegistry& registry = this->registry();
    std::vector<const TypeInfo*> result;
    for (const auto& type : registry.types()) {
        if (type->creatable() && canAttach(registry, *type, node) && missingRequirements(registry, *type, node)) {
            result.push_back(type.get());
        }
    }

    return result;
}

std::vector<const Component*> EditorSession::componentDependents(const Node& node, const Component& component) const
{
    return dependentsOf(registry(), node, component);
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
