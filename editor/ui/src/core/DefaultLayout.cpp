#include "yqlora/editor/ui/core/DefaultLayout.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace yqlora::editor {

namespace {

// Доля окна под дерево слева, инспектор справа и файлы снизу.
constexpr float kSceneTreeShare = 0.2f;
constexpr float kInspectorShare = 0.3f;
constexpr float kFilesShare = 0.25f;

} // namespace

void buildDefaultLayout(unsigned int dockspace)
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
    // Окно игры - вкладкой за окном вида: вперёд оно выходит само на Play.
    dock("Game", center);
    dock("Viewport", center);
    ImGui::DockBuilderFinish(dockspace);
}

} // namespace yqlora::editor
