#pragma once

#include "yqlora/editor/ui/core/Actions.hpp"
#include "yqlora/editor/ui/core/EditorHost.hpp"
#include "yqlora/editor/ui/core/EditorPanel.hpp"
#include "yqlora/editor/ui/dialogs/UnsavedChangesDialog.hpp"
#include "yqlora/editor/ui/panels/LogPanel.hpp"
#include "yqlora/editor/ui/panels/ProjectManagerPanel.hpp"
#include "yqlora/editor/ui/panels/ProjectSettingsPanel.hpp"
#include "yqlora/editor/ui/style/Theme.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/EditorSettings.hpp"
#include "yqlora/editor/LogBuffer.hpp"
#include "yqlora/editor/ProjectManager.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/SceneSerializer.hpp"
#include "yqlora/scripting/GameModule.hpp"

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
    void onRendererLost() override;
    // Разворачивает панель или возвращает раскладку, если развёрнута она же.
    void toggleMaximized(EditorPanel& panel);
    // Shift+Пробел: развёрнутую - вернуть, иначе развернуть ту, что в фокусе.
    void toggleMaximizedFocused();

private:
    EditorContext context();
    void openProject(Project project);
    void closeProject();
    // Сначала спросит про несохранённые изменения в текущей сцене.
    void openScene(const std::filesystem::path& path);
    void save();
    void updateTitle();

    EditorHost m_host{*this};
    // Лог для панели "Лог": подписка объявлена после буфера и уходит раньше него.
    LogBuffer m_log;
    log::Subscription m_logFeed = feedEngineLog(m_log);
    Theme m_theme;
    SceneSerializer m_serializer{registry()};
    std::unique_ptr<ProjectManager> m_projects;
    // Путь к папке пользователя известен только после запуска, см. onStart.
    std::unique_ptr<EditorSettings> m_settings;
    std::unique_ptr<ProjectManagerPanel> m_projectManager;
    GameModule m_gameModule{registry()};
    EditorSession m_session{services(), m_serializer, m_gameModule};
    ActionRegistry m_actions;
    std::vector<std::unique_ptr<EditorPanel>> m_panels;
    // Лежит в m_panels; отдельно - чтобы открывать его из меню.
    ProjectSettingsPanel* m_projectSettings = nullptr;
    // Лежит в m_panels; отдельно - чтобы открывать её по счётчику ошибок.
    LogPanel* m_logPanel = nullptr;
    // Развёрнута на всё окно; nullptr - обычная раскладка. Лежит в m_panels.
    EditorPanel* m_maximized = nullptr;
    UnsavedChangesDialog m_unsaved;
    // Курсор переносится у края окна: летит камера или тянут значение в инспекторе.
    bool m_mouseWrapping = false;
    float m_deltaSeconds = 0.0f;
    bool m_layoutChecked = false;
    bool m_newWindowsDocked = false;
    // Языки с названиями. Названия читаются из файлов, поэтому один раз при старте.
    std::vector<std::pair<std::string, std::string>> m_languages;
};

} // namespace yqlora::editor
