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

EditorSession::EditorSession(Scene& scene, const SceneSerializer& serializer)
    : m_serializer(serializer)
    , m_document(scene, serializer)
    , m_play(serializer)
{
}

const TypeRegistry& EditorSession::registry() const
{
    return m_serializer.registry();
}

bool EditorSession::open(Project project)
{
    stop();
    m_project = std::move(project);
    m_selection.clear();
    // От прошлого проекта ничего не должно остаться, даже если главная сцена
    // не прочитается: тогда проект открывается с пустой сценой.
    m_document.reset(m_project->mainScene());
    if (!m_document.load(*m_project, m_project->mainScene())) {
        log::warn("Main scene of '{}' is not loaded, starting with an empty scene", m_project->name());

        return false;
    }

    return true;
}

void EditorSession::close()
{
    stop();
    m_document.reset(0);
    m_project.reset();
    m_selection.clear();
}

std::filesystem::path EditorSession::scenePath() const
{
    return m_project ? m_document.path(*m_project) : std::filesystem::path();
}

bool EditorSession::openScene(const std::filesystem::path& path)
{
    stop();
    const ResourceId id = m_project ? m_project->resources().id(path) : 0;
    if (id == 0 || !m_document.load(*m_project, id)) {
        return false;
    }
    m_selection.clear();

    return true;
}

bool EditorSession::saveScene()
{
    return m_project && !playing() && m_document.save(*m_project);
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

void EditorSession::play()
{
    m_document.history().finishEdit();
    if (m_play.start(m_document.scene())) {
        m_selectionBeforePlay = m_selection;
    }
}

void EditorSession::stop()
{
    if (!playing()) {
        return;
    }
    const Selection selected = m_selection;
    m_play.stop();
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

bool EditorSession::execute(std::unique_ptr<Command> command)
{
    if (playing()) {
        return command && command->apply(*this);
    }

    return m_document.history().execute(*this, std::move(command));
}

bool EditorSession::undo()
{
    return !playing() && m_document.history().undo(*this);
}

bool EditorSession::redo()
{
    return !playing() && m_document.history().redo(*this);
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

        return node && std::ranges::any_of(ids, [&](NodeId other) {
                   const Node* branch = other != id ? findNode(other) : nullptr;

                   return branch && isInBranch(*branch, *node);
               });
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

    return index && canDetach(registry(), node, component)
           && execute(std::make_unique<RemoveComponentCommand>(node.id(), *index));
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
