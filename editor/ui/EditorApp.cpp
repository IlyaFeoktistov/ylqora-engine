#include "EditorApp.hpp"

#include "panels/FileSystemPanel.hpp"
#include "panels/InspectorPanel.hpp"
#include "panels/SceneTreePanel.hpp"
#include "panels/ViewportPanel.hpp"
#include "style/Icons.hpp"
#include "style/Theme.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/i18n/Localization.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/platform/Platform.hpp"

#include <imgui.h>
#include <imgui_internal.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>

namespace yqlora::editor {

EditorApp::EditorApp()
    : Application({.name = "Editor", .titleKey = "editor.title", .width = 1600, .height = 900, .ui = true})
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
        .deltaSeconds = m_deltaSeconds,
        .openScene = [this](const std::filesystem::path& path) {
            openScene(path);
        },
    };
}

void EditorApp::onStart()
{
    const auto assets = platform::executableDir() / "assets";
    theme::load(assets / "themes" / "dark.json", assets);

    for (const std::string& code : localization().availableLanguages()) {
        m_languages.emplace_back(code, localization().displayName(code));
    }

    m_projects = std::make_unique<ProjectList>(userDataDirectory() / "projects.json");
    m_projects->load();
    m_projectManager = std::make_unique<ProjectManagerPanel>(*m_projects, m_serializer);
    m_projectManager->refresh();

    registerActions();
    // В редакторе сцена стоит, пока не нажали Play.
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

    m_actions.add("scene.save", "editor.menu.save")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_S)
        // Во время игры в сцене её состояние, а не то, что правили: сохранять нечего.
        .enabledWhen([this] {
            return m_session.project() && !m_session.playing();
        })
        .onRun([this] {
            saveScene();
        });
    m_actions.add("project.close", "editor.menu.closeProject").enabledWhen(hasProject).onRun([this] {
        m_unsaved.request(m_session, [this] {
            closeProject();
        });
    });
    m_actions.add("app.quit", "editor.menu.quit").onRun([this] {
        m_unsaved.request(m_session, [this] {
            quit();
        });
    });

    m_actions.add("edit.undo", "editor.menu.undo")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_Z)
        .enabledWhen([this] {
            return m_session.canUndo();
        })
        .onRun([this] {
            m_session.undo();
        });
    m_actions.add("edit.redo", "editor.menu.redo")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_Y)
        .alternativeShortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)
        .enabledWhen([this] {
            return m_session.canRedo();
        })
        .onRun([this] {
            m_session.redo();
        });

    m_actions.add("play.toggle", "editor.play.play")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_P)
        .enabledWhen(hasProject)
        .onRun([this] {
            if (m_session.playing()) {
                m_session.stop();
            } else {
                m_session.play();
            }
        });
    m_actions.add("play.pause", "editor.play.pause").enabledWhen(playing).onRun([this] {
        m_session.setPaused(!m_session.paused());
    });
    m_actions.add("play.stop", "editor.play.stop").enabledWhen(playing).onRun([this] {
        m_session.stop();
    });
}

void EditorApp::onUpdate(float deltaSeconds)
{
    // Камера обновляется в окне сцены, там же известно, над ним ли курсор.
    m_deltaSeconds = deltaSeconds;
}

void EditorApp::onGui()
{
    ImGuizmo::BeginFrame();
    drawMenuBar();

    m_unsaved.draw(m_session);

    if (!m_session.project()) {
        if (const auto project = m_projectManager->draw(*this)) {
            openProject(*project);
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
    setSceneRunning(m_session.running());

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
    m_actions.run("app.quit");
}

void EditorApp::openProject(const Project& project)
{
    m_session.open(project);
    EditorContext frame = context();
    for (const auto& panel : m_panels) {
        panel->onProjectOpened(frame);
    }
    updateTitle();

    m_projects->markOpened(project.directory);
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
        log::info("Scene saved: {}", m_session.scenePath().generic_string());
    }
}

void EditorApp::openScene(const std::filesystem::path& path)
{
    if (path == m_session.scenePath()) {
        return;
    }
    m_unsaved.request(m_session, [this, path] {
        if (!m_session.openScene(path)) {
            log::error("Cannot open scene '{}'", path.generic_string());
        }
    });
}

void EditorApp::updateTitle()
{
    std::string title;
    if (m_session.project()) {
        const std::u8string scene = m_session.scenePath().filename().u8string();
        title = m_session.project()->name + " - " + std::string(scene.begin(), scene.end());
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
            m_actions.menuItem("scene.save");
            m_actions.menuItem("project.close");
            ImGui::Separator();
        }
        m_actions.menuItem("app.quit");
        ImGui::EndMenu();
    }
    if (m_session.project() && ImGui::BeginMenu(tr("editor.menu.edit").c_str())) {
        m_actions.menuItem("edit.undo");
        m_actions.menuItem("edit.redo");
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
    const float buttonWidth = ImGui::CalcTextSize(icons::kPlay).x + style.FramePadding.x * 2.0f;
    const float width = buttonWidth * 3.0f + style.ItemSpacing.x * 2.0f;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);

    const Vec4& accent = theme::colors().playMode;
    const bool playing = m_session.playing();
    if (playing) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPlay)) {
        m_actions.run("play.toggle");
    }
    if (playing) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip(
        "%s (%s)",
        tr(playing ? "editor.play.stop" : "editor.play.play").c_str(),
        m_actions.shortcutText("play.toggle").c_str()
    );

    ImGui::BeginDisabled(!playing);
    const bool paused = m_session.paused();
    if (paused) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPause)) {
        m_actions.run("play.pause");
    }
    if (paused) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.pause").c_str());
    if (ImGui::SmallButton(icons::kStop)) {
        m_actions.run("play.stop");
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
    const ImGuiID left = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.2f, nullptr, &center);
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.3f, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.25f, nullptr, &center);

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
