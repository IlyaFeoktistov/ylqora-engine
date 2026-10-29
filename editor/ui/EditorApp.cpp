#include "EditorApp.hpp"

#include "core/ActionIds.hpp"
#include "core/DefaultLayout.hpp"
#include "core/EditorActions.hpp"
#include "menu/MainMenu.hpp"

#include "panels/FileSystemPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneTreePanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "style/Icons.hpp"
#include "style/Theme.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/i18n/Localization.hpp"
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
          .ui = true,
      })
{
    m_panels.push_back(std::make_unique<SceneTreePanel>());
    m_panels.push_back(std::make_unique<InspectorPanel>());
    m_panels.push_back(std::make_unique<FileSystemPanel>());
    m_panels.push_back(std::make_unique<ViewportPanel>());
}

EditorContext EditorApp::context()
{
    return {
        .app = *this,
        .session = m_session,
        .registry = registry(),
        .serializer = m_serializer,
        .actions = m_actions,
        .theme = m_theme,
        .settings = *m_settings,
        .deltaSeconds = m_deltaSeconds,
        .openScene = [this](const std::filesystem::path& path) {
            openScene(path);
        },
    };
}

void EditorApp::onStart()
{
    const auto assets = assetsDirectory();
    m_theme.load(assets / kThemeFile, assets);

    for (const std::string& code : localization().availableLanguages()) {
        m_languages.emplace_back(code, localization().displayName(code));
    }

    m_projects = std::make_unique<ProjectManager>(userDataDirectory() / kProjectListFile, m_serializer);
    m_settings = std::make_unique<EditorSettings>(userDataDirectory() / kEditorSettingsFile);
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
        }
    );
    // Своя сцена приложения у редактора не обновляется: игра идёт в копии, см. EditorSession::play.
    setSceneRunning(false);
}

void EditorApp::onUpdate(float deltaSeconds)
{
    m_session.update(deltaSeconds);
    // Камера обновляется в окне сцены, там же известно, над ним ли курсор.
    m_deltaSeconds = deltaSeconds;
}

void EditorApp::onGui()
{
    ImGuizmo::BeginFrame();
    drawMainMenu({
        .app = *this,
        .session = m_session,
        .actions = m_actions,
        .panels = m_panels,
        .languages = m_languages,
        .theme = m_theme,
    });

    m_unsaved.draw(m_session);

    if (!m_session.project()) {
        if (auto project = m_projectManager->draw(*this)) {
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
    services().setProject(&*m_session.project());
    EditorContext frame = context();
    for (const auto& panel : m_panels) {
        panel->onProjectOpened(frame);
    }
    updateTitle();

    m_projects->markOpened(directory);
}

void EditorApp::closeProject()
{
    services().setProject(nullptr);
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
    std::string title;
    if (m_session.project()) {
        title = m_session.project()->name() + " - " + toUtf8(m_session.scenePath().filename());
        if (m_session.document().modified()) {
            title += " *";
        }
    }
    if (title != m_title) {
        m_title = title;
        setTitleDetail(title);
    }
}

} // namespace yqlora::editor
