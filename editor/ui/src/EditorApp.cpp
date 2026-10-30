#include "yqlora/editor/ui/EditorApp.hpp"

#include "yqlora/editor/ui/core/ActionIds.hpp"
#include "yqlora/editor/ui/core/DefaultLayout.hpp"
#include "yqlora/editor/ui/core/EditorActions.hpp"
#include "yqlora/editor/ui/menu/MainMenu.hpp"

#include "yqlora/editor/ui/panels/FileSystemPanel.hpp"
#include "yqlora/editor/ui/panels/GamePanel.hpp"
#include "yqlora/editor/ui/panels/InspectorPanel.hpp"
#include "yqlora/editor/ui/panels/SceneTreePanel.hpp"
#include "yqlora/editor/ui/panels/ViewportPanel.hpp"
#include "yqlora/editor/ui/style/Icons.hpp"
#include "yqlora/editor/ui/style/Theme.hpp"

#include "yqlora/core/AppPaths.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/i18n/Language.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/platform/Platform.hpp"

#include <imgui.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>

namespace yqlora::editor {

namespace {

// Из имени строится папка настроек редактора, поэтому латиница без пробелов.
constexpr const char* kApplicationName = "Editor";
constexpr const char* kTitleKey = "editor.title";
constexpr int kWindowWidth = 1600;
constexpr int kWindowHeight = 900;
constexpr const char* kThemeFile = "themes/dark.json";
constexpr const char* kProjectListFile = "projects.json";
constexpr const char* kEditorSettingsFile = "editor.json";

} // namespace

EditorApp::EditorApp()
    : Application({
          .name = kApplicationName,
          .titleKey = kTitleKey,
          .width = kWindowWidth,
          .height = kWindowHeight,
          .mode = ApplicationMode::Tool,
      })
{
    m_panels.push_back(std::make_unique<SceneTreePanel>());
    m_panels.push_back(std::make_unique<InspectorPanel>());
    m_panels.push_back(std::make_unique<FileSystemPanel>());
    m_panels.push_back(std::make_unique<ViewportPanel>());
    m_panels.push_back(std::make_unique<GamePanel>());
    auto projectSettings = std::make_unique<ProjectSettingsPanel>();
    m_projectSettings = projectSettings.get();
    m_panels.push_back(std::move(projectSettings));
}

EditorContext EditorApp::context()
{
    return {
        .host = m_host,
        .session = m_session,
        .registry = registry(),
        .serializer = m_serializer,
        .actions = m_actions,
        .theme = m_theme,
        .settings = *m_settings,
        .deltaSeconds = m_deltaSeconds,
        .openScene =
            [this](const std::filesystem::path& path) {
                openScene(path);
            },
        .openProjectSettings =
            [this](std::string_view section) {
                m_projectSettings->open(section);
            },
    };
}

void EditorApp::onStart()
{
    const auto assets = assetsDirectory();
    const auto userData = userDataDirectory(kApplicationName);
    m_theme.load(assets / kThemeFile, assets);

    const Localization& localization = language().localization();
    for (const std::string& code : localization.availableLanguages()) {
        m_languages.emplace_back(code, localization.displayName(code));
    }

    m_projects = std::make_unique<ProjectManager>(userData / kProjectListFile, m_serializer);
    m_settings = std::make_unique<EditorSettings>(userData / kEditorSettingsFile);
    m_settings->load();
    m_projectManager = std::make_unique<ProjectManagerPanel>(*m_projects, m_theme);

    registerEditorActions(
        m_actions,
        m_session,
        {
            .saveScene =
                [this] {
                    saveScene();
                },
            .closeProject =
                [this] {
                    m_unsaved.request(m_session, [this] {
                        closeProject();
                    });
                },
            .quit =
                [this] {
                    m_unsaved.request(m_session, [this] {
                        quit();
                    });
                },
            .openProjectSettings =
                [this] {
                    m_projectSettings->open();
                },
        }
    );
}

void EditorApp::onUpdate(float deltaSeconds)
{
    m_session.update(deltaSeconds, input());
    // Камера обновляется в окне сцены, там же известно, над ним ли курсор.
    m_deltaSeconds = deltaSeconds;
}

void EditorApp::onGui()
{
    ImGuizmo::BeginFrame();
    drawMainMenu({
        .host = m_host,
        .session = m_session,
        .actions = m_actions,
        .panels = m_panels,
        .languages = m_languages,
        .theme = m_theme,
    });

    m_unsaved.draw(m_session);

    if (!m_session.project()) {
        if (auto project = m_projectManager->draw(m_host)) {
            openProject(std::move(*project));
        }

        return;
    }

    m_actions.handleShortcuts();

    const ImGuiID dockspace = ImGui::DockSpaceOverViewport();
    if (!m_layoutChecked) {
        m_layoutChecked = true;
        buildDefaultLayout(dockspace);
    }

    EditorContext frame = context();
    for (const auto& panel : m_panels) {
        if (panel->visible()) {
            panel->draw(frame);
        }
    }
    updateTitle();

    // Ничего не тянут и не печатают: следующая правка будет отдельным шагом отмены.
    if (!ImGui::IsAnyItemActive() && !ImGuizmo::IsUsing()) {
        m_session.finishEdit();
    }

    if (frame.mouseWrapping != m_mouseWrapping) {
        m_mouseWrapping = frame.mouseWrapping;
        setMouseWrapping(frame.mouseWrapping);
    }
}

void EditorApp::onCloseRequested()
{
    m_actions.run(actions::kQuit);
}

void EditorApp::openProject(Project project)
{
    const std::filesystem::path directory = project.directory();
    m_session.open(std::move(project));
    EditorContext frame = context();
    for (const auto& panel : m_panels) {
        panel->onProjectOpened(frame);
    }
    updateTitle();

    m_projects->markOpened(directory);
}

void EditorApp::closeProject()
{
    m_session.close();
    m_mouseWrapping = false;
    setMouseWrapping(false);
    updateTitle();
    m_projects->refresh();
}

void EditorApp::saveScene()
{
    if (m_session.saveScene()) {
        log::info("Scene saved: {}", toUtf8(m_session.scenePath()));
    }
}

void EditorApp::openScene(const std::filesystem::path& path)
{
    if (path == m_session.scenePath()) {
        return;
    }
    m_unsaved.request(m_session, [this, path] {
        if (!m_session.openScene(path)) {
            log::error("Cannot open scene '{}'", toUtf8(path));
        }
    });
}

void EditorApp::updateTitle()
{
    setTitleDetail(windowTitle(m_session));
}

} // namespace yqlora::editor
