#include "EditorApp.hpp"

#include "Icons.hpp"
#include "Theme.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/i18n/Localization.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/platform/Platform.hpp"

#include <imgui.h>
#include <imgui_internal.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>

namespace yqlora {

EditorApp::EditorApp()
    : Application({.name = "Editor", .titleKey = "editor.title", .width = 1600, .height = 900, .ui = true})
{
    registerEngineTypes(m_registry);
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

    // В редакторе сцена стоит, пока не нажали Play.
    setSceneRunning(false);
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

    drawUnsavedDialog();

    if (!m_session.project()) {
        if (const auto project = m_projectManager->draw(*this)) {
            openProject(*project);
        }

        return;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
        saveScene();
    }
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_P, ImGuiInputFlags_RouteGlobal)) {
        togglePlay();
    }

    const ImGuiID dockspace = ImGui::DockSpaceOverViewport();
    if (!m_layoutChecked) {
        m_layoutChecked = true;
        buildDefaultLayout(dockspace);
    }

    m_sceneTree.draw(m_session, m_registry);
    m_inspector.draw(m_session, m_registry);
    m_files.draw(m_session, m_serializer);
    m_viewport.draw(*this, m_session, m_deltaSeconds);
    if (const auto scene = m_files.takeOpenRequest(); scene && *scene != m_session.scenePath()) {
        m_pendingScene = *scene;
        request(PendingAction::OpenScene);
    }
    updateTitle();
    setSceneRunning(m_session.running());

    const bool wrapping = m_viewport.camera().flying() || m_inspector.draggingValue();
    if (wrapping != m_mouseWrapping) {
        m_mouseWrapping = wrapping;
        setMouseWrapping(wrapping);
    }
}

void EditorApp::onCloseRequested()
{
    request(PendingAction::Quit);
}

void EditorApp::openProject(const Project& project)
{
    m_session.open(project);
    // Наискосок сверху, как вид по умолчанию в Blender: видно все три оси сразу.
    m_viewport.camera().setPosition({6.0f, 4.5f, 6.0f});
    m_viewport.camera().lookAt({0.0f, 0.5f, 0.0f});
    m_files.refresh();
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

void EditorApp::request(PendingAction action)
{
    if (m_session.modified()) {
        m_pending = action;
        m_unsavedRequested = true;

        return;
    }
    perform(action);
}

void EditorApp::perform(PendingAction action)
{
    switch (action) {
    case PendingAction::OpenScene:
        if (!m_session.openScene(m_pendingScene)) {
            log::error("Cannot open scene '{}'", m_pendingScene.generic_string());
        }
        break;
    case PendingAction::CloseProject:
        closeProject();
        break;
    case PendingAction::Quit:
        quit();
        break;
    case PendingAction::None:
        break;
    }
}

void EditorApp::drawUnsavedDialog()
{
    const std::string title = tr("editor.unsaved.title") + "###Unsaved";
    if (m_unsavedRequested) {
        ImGui::OpenPopup(title.c_str());
        m_unsavedRequested = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::TextUnformatted(tr("editor.unsaved.question").c_str());
    ImGui::Separator();

    if (ImGui::Button(tr("editor.unsaved.save").c_str())) {
        // Не сохранилось - остаёмся, чтобы изменения не пропали.
        if (m_session.saveScene()) {
            perform(std::exchange(m_pending, PendingAction::None));
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.unsaved.discard").c_str())) {
        perform(std::exchange(m_pending, PendingAction::None));
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.common.cancel").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_pending = PendingAction::None;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
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
            // Во время игры в сцене её состояние, а не то, что правили: сохранять нечего.
            if (ImGui::MenuItem(tr("editor.menu.save").c_str(), "Ctrl+S", false, !m_session.playing())) {
                saveScene();
            }
            if (ImGui::MenuItem(tr("editor.menu.closeProject").c_str())) {
                request(PendingAction::CloseProject);
            }
            ImGui::Separator();
        }
        if (ImGui::MenuItem(tr("editor.menu.quit").c_str())) {
            request(PendingAction::Quit);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr("editor.menu.view").c_str())) {
        drawLanguageMenu();
        ImGui::EndMenu();
    }
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
        togglePlay();
    }
    if (playing) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s (Ctrl+P)", tr(playing ? "editor.play.stop" : "editor.play.play").c_str());

    ImGui::BeginDisabled(!playing);
    const bool paused = m_session.paused();
    if (paused) {
        ImGui::PushStyleColor(ImGuiCol_Text, theme::toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPause)) {
        m_session.setPaused(!paused);
    }
    if (paused) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.pause").c_str());
    if (ImGui::SmallButton(icons::kStop)) {
        m_session.stop();
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.stop").c_str());
    ImGui::EndDisabled();
}

void EditorApp::togglePlay()
{
    if (m_session.playing()) {
        m_session.stop();
    } else {
        m_session.play();
    }
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
    ImGui::DockBuilderDockWindow("###Scene", left);
    ImGui::DockBuilderDockWindow("###Inspector", right);
    ImGui::DockBuilderDockWindow("###FileSystem", bottom);
    ImGui::DockBuilderDockWindow("###Viewport", center);
    ImGui::DockBuilderFinish(dockspace);
}

} // namespace yqlora
