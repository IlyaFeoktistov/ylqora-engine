#include "EditorApp.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/i18n/Localization.hpp"
#include "yqlora/i18n/Translate.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace yqlora {

EditorApp::EditorApp()
    : Application({.name = "Editor", .titleKey = "editor.title", .width = 1600, .height = 900, .ui = true})
{
    registerEngineTypes(m_registry);
}

void EditorApp::onStart()
{
    for (const std::string& code : localization().availableLanguages()) {
        m_languages.emplace_back(code, localization().displayName(code));
    }

    m_projects = std::make_unique<ProjectList>(userDataDirectory() / "projects.json");
    m_projects->load();
    m_projectManager = std::make_unique<ProjectManagerPanel>(*m_projects, m_serializer);
    m_projectManager->refresh();
}

void EditorApp::onUpdate(float deltaSeconds)
{
    // Камера обновляется в окне сцены, там же известно, над ним ли курсор.
    m_deltaSeconds = deltaSeconds;
}

void EditorApp::onGui()
{
    drawMenuBar();

    if (!m_project) {
        if (const auto project = m_projectManager->draw(*this)) {
            openProject(*project);
        }

        return;
    }

    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
        saveScene();
    }

    const ImGuiID dockspace = ImGui::DockSpaceOverViewport();
    if (!m_layoutChecked) {
        m_layoutChecked = true;
        buildDefaultLayout(dockspace);
    }

    m_sceneTree.draw(scene(), m_selection);
    m_viewport.draw(*this, scene(), m_selection, m_deltaSeconds);
}

void EditorApp::openProject(const Project& project)
{
    // Загрузка при ошибке оставляет сцену как была, поэтому сначала очищаем:
    // от прошлого проекта ничего не должно остаться.
    scene().setRoot(nullptr);
    if (!m_serializer.load(scene(), project.mainScenePath())) {
        log::warn("Main scene of '{}' is not loaded, starting with an empty scene", project.name);
    }

    m_project = project;
    m_selection.clear();
    m_viewport.camera().setPosition({0.0f, 2.5f, 6.0f});
    m_viewport.camera().lookAt({0.0f, 0.5f, 0.0f});
    setTitleDetail(project.name);

    m_projects->markOpened(project.directory);
    if (!m_projects->save()) {
        log::error("Cannot save the project list");
    }
}

void EditorApp::closeProject()
{
    scene().setRoot(nullptr);
    m_project.reset();
    m_selection.clear();
    setTitleDetail({});
    m_projectManager->refresh();
}

void EditorApp::saveScene()
{
    if (!m_project) {
        return;
    }
    if (m_serializer.save(scene(), m_project->mainScenePath())) {
        log::info("Scene saved: {}", m_project->mainScene.generic_string());
    }
}

void EditorApp::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu(tr("editor.menu.file").c_str())) {
        if (m_project) {
            if (ImGui::MenuItem(tr("editor.menu.save").c_str(), "Ctrl+S")) {
                saveScene();
            }
            if (ImGui::MenuItem(tr("editor.menu.closeProject").c_str())) {
                closeProject();
            }
            ImGui::Separator();
        }
        if (ImGui::MenuItem(tr("editor.menu.quit").c_str())) {
            quit();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu(tr("editor.menu.view").c_str())) {
        drawLanguageMenu();
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
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
    const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.25f, nullptr, &center);
    const ImGuiID bottom = ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.25f, nullptr, &center);

    // Окна привязываются по ID после ###, он не зависит от языка интерфейса.
    ImGui::DockBuilderDockWindow("###Scene", left);
    ImGui::DockBuilderDockWindow("###Inspector", right);
    ImGui::DockBuilderDockWindow("###FileSystem", bottom);
    ImGui::DockBuilderDockWindow("###Viewport", center);
    ImGui::DockBuilderFinish(dockspace);
}

} // namespace yqlora
