#pragma once

#include "core/Actions.hpp"
#include "core/EditorHost.hpp"
#include "core/EditorPanel.hpp"
#include "dialogs/UnsavedChangesDialog.hpp"
#include "panels/ProjectManagerPanel.hpp"
#include "style/Theme.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/EditorSettings.hpp"
#include "yqlora/editor/ProjectManager.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace yqlora::editor {

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
    EditorContext context();
    void openProject(Project project);
    void closeProject();
    // Сначала спросит про несохранённые изменения в текущей сцене.
    void openScene(const std::filesystem::path& path);
    void saveScene();
    void updateTitle();

    EditorHost m_host{*this};
    Theme m_theme;
    SceneSerializer m_serializer{registry()};
    std::unique_ptr<ProjectManager> m_projects;
    // Путь к папке пользователя известен только после запуска, см. onStart.
    std::unique_ptr<EditorSettings> m_settings;
    std::unique_ptr<ProjectManagerPanel> m_projectManager;
    EditorSession m_session{services(), m_serializer};
    ActionRegistry m_actions;
    std::vector<std::unique_ptr<EditorPanel>> m_panels;
    UnsavedChangesDialog m_unsaved;
    // Курсор переносится у края окна: летит камера или тянут значение в инспекторе.
    bool m_mouseWrapping = false;
    float m_deltaSeconds = 0.0f;
    bool m_layoutChecked = false;
    // Языки с названиями. Названия читаются из файлов, поэтому один раз при старте.
    std::vector<std::pair<std::string, std::string>> m_languages;
};

} // namespace yqlora::editor
