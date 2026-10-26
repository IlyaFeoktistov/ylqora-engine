#include "EditorApp.hpp"

#include "yqlora/i18n/Localization.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/MeshInstance3D.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace yqlora {

namespace {

// Пока нет проектов, редактор открывается с примером сцены.
void buildExampleScene(Scene& scene)
{
    auto& root = scene.createRoot<Node3D>("World");

    auto& camera = root.addChild<Camera3D>("MainCamera");
    camera.transform().setPosition({0.0f, 2.0f, 5.0f});
    camera.transform().lookAt({0.0f, 0.0f, 0.0f});
    scene.setActiveCamera(&camera);

    auto& cube = root.addChild<MeshInstance3D>("Cube");
    cube.transform().setPosition({0.0f, 0.5f, 0.0f});
    cube.setColor({0.85f, 0.45f, 0.3f, 1.0f});

    auto& sphere = cube.addChild<MeshInstance3D>("Sphere");
    sphere.setMesh("sphere");
    sphere.transform().setPosition({1.5f, 0.0f, 0.0f});
    sphere.setColor({0.35f, 0.65f, 0.9f, 1.0f});

    auto& box = root.addChild<MeshInstance3D>("Box");
    box.transform().setPosition({-1.5f, 0.35f, 0.5f});
    box.transform().setScale({0.7f, 0.7f, 0.7f});
    box.transform().setRotationDegrees({0.0f, 30.0f, 0.0f});
    box.setColor({0.5f, 0.8f, 0.45f, 1.0f});
}

} // namespace

EditorApp::EditorApp()
    : Application({.name = "Editor", .titleKey = "editor.title", .width = 1600, .height = 900, .ui = true})
{
}

void EditorApp::onStart()
{
    for (const std::string& code : localization().availableLanguages()) {
        m_languages.emplace_back(code, localization().displayName(code));
    }

    buildExampleScene(scene());
    m_viewport.camera().setPosition({0.0f, 2.5f, 6.0f});
    m_viewport.camera().lookAt({0.0f, 0.5f, 0.0f});
}

void EditorApp::onUpdate(float deltaSeconds)
{
    // Камера обновляется в окне сцены, там же известно, над ним ли курсор.
    m_deltaSeconds = deltaSeconds;
}

void EditorApp::onGui()
{
    drawMenuBar();

    const ImGuiID dockspace = ImGui::DockSpaceOverViewport();
    if (!m_layoutChecked) {
        m_layoutChecked = true;
        buildDefaultLayout(dockspace);
    }

    m_sceneTree.draw(scene(), m_selection);
    m_viewport.draw(*this, scene(), m_selection, m_deltaSeconds);
}

void EditorApp::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu(tr("editor.menu.file").c_str())) {
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
