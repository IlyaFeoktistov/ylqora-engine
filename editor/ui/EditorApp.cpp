#include "EditorApp.hpp"

#include "core/ActionIds.hpp"

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
#include <imgui_internal.h>
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

// Раскладка по умолчанию: доля окна под дерево слева, инспектор справа и файлы снизу.
constexpr float kSceneTreeShare = 0.2f;
constexpr float kInspectorShare = 0.3f;
constexpr float kFilesShare = 0.25f;

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
    registerEngineTypes(m_registry);

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
        .registry = m_registry,
        .serializer = m_serializer,
        .actions = m_actions,
        .theme = m_theme,
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

    m_projects = std::make_unique<ProjectList>(userDataDirectory() / kProjectListFile);
    m_projects->load();
    m_projectManager = std::make_unique<ProjectManagerPanel>(*m_projects, m_serializer, m_theme);
    m_projectManager->refresh();

    registerActions();
    // Своя сцена приложения у редактора не обновляется: игра идёт в копии, см. EditorSession::play.
    setSceneRunning(false);
}

void EditorApp::registerActions()
{
    const auto hasProject = [this] {
        return m_session.project().has_value();
    };
    const auto playing = [this] {
        return m_session.playing();
    };

    m_actions.add(actions::kSaveScene, "editor.menu.save")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_S)
        // Во время игры в сцене её состояние, а не то, что правили: сохранять нечего.
        .enabledWhen([this] {
            return m_session.project() && !m_session.playing();
        })
        .onRun([this] {
            saveScene();
        });
    m_actions.add(actions::kCloseProject, "editor.menu.closeProject").enabledWhen(hasProject).onRun([this] {
        m_unsaved.request(m_session, [this] {
            closeProject();
        });
    });
    m_actions.add(actions::kQuit, "editor.menu.quit").onRun([this] {
        m_unsaved.request(m_session, [this] {
            quit();
        });
    });

    m_actions.add(actions::kUndo, "editor.menu.undo")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_Z)
        .enabledWhen([this] {
            return m_session.canUndo();
        })
        .onRun([this] {
            m_session.undo();
        });
    m_actions.add(actions::kRedo, "editor.menu.redo")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_Y)
        .alternativeShortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)
        .enabledWhen([this] {
            return m_session.canRedo();
        })
        .onRun([this] {
            m_session.redo();
        });

    m_actions.add(actions::kTogglePlay, "editor.play.play")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_P)
        .enabledWhen(hasProject)
        .onRun([this] {
            if (m_session.playing()) {
                m_session.stop();
            } else {
                m_session.play();
            }
        });
    m_actions.add(actions::kPause, "editor.play.pause").enabledWhen(playing).onRun([this] {
        m_session.setPaused(!m_session.paused());
    });
    m_actions.add(actions::kStop, "editor.play.stop").enabledWhen(playing).onRun([this] {
        m_session.stop();
    });
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
    drawMenuBar();

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
    EditorContext frame = context();
    for (const auto& panel : m_panels) {
        panel->onProjectOpened(frame);
    }
    updateTitle();

    m_projects->markOpened(directory);
    if (!m_projects->save()) {
        log::error("Cannot save the project list");
    }
}

void EditorApp::closeProject()
{
    m_session.close();
    m_mouseWrapping = false;
    setMouseWrapping(false);
    updateTitle();
    m_projectManager->refresh();
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
        if (m_session.modified()) {
            title += " *";
        }
    }
    if (title != m_title) {
        m_title = title;
        setTitleDetail(title);
    }
}

void EditorApp::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu(tr("editor.menu.file").c_str())) {
        if (m_session.project()) {
            m_actions.menuItem(actions::kSaveScene);
            m_actions.menuItem(actions::kCloseProject);
            ImGui::Separator();
        }
        m_actions.menuItem(actions::kQuit);
        ImGui::EndMenu();
    }
    if (m_session.project() && ImGui::BeginMenu(tr("editor.menu.edit").c_str())) {
        m_actions.menuItem(actions::kUndo);
        m_actions.menuItem(actions::kRedo);
        ImGui::EndMenu();
    }
    drawViewMenu();
    if (m_session.project()) {
        drawPlayControls();
    }
    ImGui::EndMainMenuBar();
}

void EditorApp::drawPlayControls()
{
    const ImGuiStyle& style = ImGui::GetStyle();
    // Play, Pause и Stop одинаковой ширины.
    constexpr float kButtonCount = 3.0f;
    const float buttonWidth = ImGui::CalcTextSize(icons::kPlay).x + style.FramePadding.x * 2.0f;
    const float width = buttonWidth * kButtonCount + style.ItemSpacing.x * (kButtonCount - 1.0f);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);

    const Vec4& accent = m_theme.colors().playMode;
    const bool playing = m_session.playing();
    if (playing) {
        ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPlay)) {
        m_actions.run(actions::kTogglePlay);
    }
    if (playing) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip(
        "%s (%s)",
        tr(playing ? "editor.play.stop" : "editor.play.play").c_str(),
        m_actions.shortcutText(actions::kTogglePlay).c_str()
    );

    ImGui::BeginDisabled(!playing);
    const bool paused = m_session.paused();
    if (paused) {
        ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPause)) {
        m_actions.run(actions::kPause);
    }
    if (paused) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.pause").c_str());
    if (ImGui::SmallButton(icons::kStop)) {
        m_actions.run(actions::kStop);
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.stop").c_str());
    ImGui::EndDisabled();
}

void EditorApp::drawViewMenu()
{
    if (!ImGui::BeginMenu(tr("editor.menu.view").c_str())) {
        return;
    }
    if (m_session.project()) {
        for (const auto& panel : m_panels) {
            if (panel->closable() && ImGui::MenuItem(tr(panel->titleKey()).c_str(), nullptr, panel->visible())) {
                panel->setVisible(!panel->visible());
            }
        }
        ImGui::Separator();
    }
    drawLanguageMenu();
    ImGui::EndMenu();
}

void EditorApp::drawLanguageMenu()
{
    if (!ImGui::BeginMenu(tr("editor.menu.language").c_str())) {
        return;
    }
    for (const auto& [code, name] : m_languages) {
        const bool current = code == localization().language();
        if (ImGui::MenuItem(name.c_str(), nullptr, current) && !current) {
            setLanguage(code);
        }
    }
    ImGui::EndMenu();
}

void EditorApp::buildDefaultLayout(unsigned int dockspace)
{
    // Раскладка уже есть в imgui.ini: пользователь мог её поменять, не трогаем.
    const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace);
    if (node && node->IsSplitNode()) {
        return;
    }

    ImGui::DockBuilderRemoveNode(dockspace);
    ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
    ImGui::DockBuilderSetNodeSize(dockspace, ImGui::GetMainViewport()->WorkSize);

    ImGuiID center = dockspace;
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, kSceneTreeShare, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, kInspectorShare, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, kFilesShare, nullptr, &center);

    // Окна привязываются по ID после ###, он не зависит от языка интерфейса.
    const auto dock = [](const char* id, ImGuiID target) {
        ImGui::DockBuilderDockWindow((std::string("###") + id).c_str(), target);
    };
    dock("Scene", left);
    dock("Inspector", right);
    dock("FileSystem", bottom);
    dock("Viewport", center);
    ImGui::DockBuilderFinish(dockspace);
}

} // namespace yqlora::editor
