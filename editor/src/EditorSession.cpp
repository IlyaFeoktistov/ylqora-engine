#include "yqlora/editor/EditorSession.hpp"

#include "yqlora/editor/SceneCommands.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/project/GeneralSettings.hpp"
#include "yqlora/reflection/ComponentRules.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Node3D.hpp"
#include "yqlora/scene/NodeTree.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <algorithm>
#include <optional>
#include <string>

namespace yqlora::editor {

EditorSession::EditorSession(EngineServices& services, const SceneSerializer& serializer)
    : m_services(services)
    , m_serializer(serializer)
    , m_document(services, serializer)
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
    m_services.setProject(&*m_project);
    m_selection.clear();
    m_history.clear();
    // От прошлого проекта ничего не должно остаться, даже если стартовая
    // сцена не прочитается: тогда проект открывается с пустой сценой.
    const ResourceId startup = m_project->settings().get<GeneralSettings>().startupScene.id;
    m_document.reset(startup);
    if (!m_document.load(*m_project, startup)) {
        log::warn("Startup scene of '{}' is not loaded, starting with an empty scene", m_project->name());

        return false;
    }

    return true;
}

void EditorSession::close()
{
    stop();
    m_document.reset(0);
    m_history.clear();
    m_services.setProject(nullptr);
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
    m_history.clear();
    m_selection.clear();

    return true;
}

bool EditorSession::saveScene()
{
    if (!m_project || playing() || !m_document.save(*m_project)) {
        return false;
    }
    m_history.markSaved();

    return true;
}

bool EditorSession::canUndo() const
{
    return !playing() && m_history.canUndo();
}

bool EditorSession::canRedo() const
{
    return !playing() && m_history.canRedo();
}

bool EditorSession::undo()
{
    return !playing() && m_history.undo(*this);
}

bool EditorSession::redo()
{
    return !playing() && m_history.redo(*this);
}

bool EditorSession::execute(std::unique_ptr<Command> command)
{
    if (playing() && command && command->changesScene()) {
        return command->apply(*this);
    }

    return m_history.execute(*this, std::move(command));
}

void EditorSession::play()
{
    m_history.finishEdit();
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

Node* EditorSession::findNode(NodeId id)
{
    Scene& current = scene();

    return current.root() && id != 0 ? yqlora::findNode(*current.root(), id) : nullptr;
}

Node* EditorSession::selectedNode()
{
    return findNode(m_selection.selected());
}

std::string windowTitle(const EditorSession& session)
{
    if (!session.project()) {
        return {};
    }
    std::string title = session.project()->name() + " - " + toUtf8(session.scenePath().filename());
    if (session.sceneModified()) {
        title += " *";
    }

    return title;
}

} // namespace yqlora::editor
