#include "yqlora/editor/ui/core/DefaultLayout.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace yqlora::editor {

namespace {

std::string windowName(const char* id)
{
    return std::string("###") + id;
}

// Окно и сосед, к которому оно встаёт вкладкой.
struct NewWindow {
    const char* id;
    const char* beside;
};

constexpr NewWindow kNewWindows[] = {
    {"Log", "FileSystem"},
};

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
        ImGui::DockBuilderDockWindow(windowName(id).c_str(), target);
    };
    dock("Scene", left);
    dock("Inspector", right);
    // Лог раньше файлов: впереди остаются файлы.
    dock("Log", bottom);
    dock("FileSystem", bottom);
    // Окно игры - вкладкой за окном вида: вперёд оно выходит само на Play.
    dock("Game", center);
    dock("Viewport", center);
    ImGui::DockBuilderFinish(dockspace);
}

bool dockNewWindows()
{
    for (const NewWindow& entry : kNewWindows) {
        const std::string name = windowName(entry.id);
        ImGuiWindow* window = ImGui::FindWindowByName(name.c_str());
        const ImGuiWindow* beside = ImGui::FindWindowByName(windowName(entry.beside).c_str());
        if (!window || !beside) {
            return false;
        }
        // Есть в imgui.ini - пользователь уже видел окно и сам решил, где ему быть.
        if (window->DockId == 0 && beside->DockId != 0 && !ImGui::FindWindowSettingsByID(window->ID)) {
            ImGui::DockBuilderDockWindow(name.c_str(), beside->DockId);
        }
    }

    return true;
}

} // namespace yqlora::editor
