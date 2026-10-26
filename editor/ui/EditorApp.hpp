#pragma once

#include "InspectorPanel.hpp"
#include "ProjectManagerPanel.hpp"
#include "SceneTreePanel.hpp"
#include "ViewportPanel.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/Project.hpp"
#include "yqlora/editor/ProjectList.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace yqlora {

// Движок с редактором. При запуске показывает менеджер проектов,
// после выбора проекта - редактор его сцены.
class EditorApp final : public Application {
public:
    EditorApp();

protected:
    void onStart() override;
    void onUpdate(float deltaSeconds) override;
    void onGui() override;
    void onCloseRequested() override;

private:
    // Что сделать после ответа на вопрос о несохранённых изменениях.
    enum class PendingAction {
        None,
        CloseProject,
        Quit,
    };

    void openProject(const Project& project);
    void closeProject();
    void saveScene();
    // Если есть несохранённые изменения, сначала спрашивает, сохранить ли их.
    void request(PendingAction action);
    void perform(PendingAction action);
    void drawUnsavedDialog();
    void updateTitle();

    void drawMenuBar();
    void drawLanguageMenu();
    // При первом запуске раскладывает окна: дерево слева, инспектор справа,
    // файлы снизу, сцена в центре. Дальше раскладку помнит imgui.ini.
    void buildDefaultLayout(unsigned int dockspace);

    TypeRegistry m_registry;
    SceneSerializer m_serializer{m_registry};
    std::unique_ptr<ProjectList> m_projects;
    std::unique_ptr<ProjectManagerPanel> m_projectManager;
    EditorSession m_session{scene(), m_serializer};

    SceneTreePanel m_sceneTree;
    InspectorPanel m_inspector;
    ViewportPanel m_viewport;
    PendingAction m_pending = PendingAction::None;
    bool m_unsavedRequested = false;
    // Курсор переносится у края окна: летит камера или тянут значение в инспекторе.
    bool m_mouseWrapping = false;
    // Что сейчас в заголовке окна, чтобы не менять его каждый кадр.
    bool m_titleModified = false;
    float m_deltaSeconds = 0.0f;
    bool m_layoutChecked = false;
    // Языки с названиями. Названия читаются из файлов, поэтому один раз при старте.
    std::vector<std::pair<std::string, std::string>> m_languages;
};

} // namespace yqlora
