#include "yqlora/editor/ui/core/EditorPanel.hpp"

#include "yqlora/editor/ui/widgets/TitleBarButton.hpp"

namespace yqlora::editor {

namespace {

// Развёрнутая панель - другое окно ImGui (свой ID): обычное остаётся в
// раскладке на своём месте и вернётся туда же.
constexpr const char* kMaximizedIdSuffix = ".maximized";
constexpr const char* kPanelMenu = "##panelMenu";
// Своё семейство окон: развёрнутое окно не пристыкуется к раскладке.
constexpr ImGuiID kMaximizedWindowClass = 1;

} // namespace

bool EditorPanel::beginWindow(EditorContext& context, ImGuiWindowFlags flags)
{
    const bool isMaximized = maximized(context);
    std::string name = title();
    if (isMaximized) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGuiWindowClass windowClass;
        windowClass.ClassId = kMaximizedWindowClass;
        windowClass.DockingAllowUnclassed = false;
        ImGui::SetNextWindowClass(&windowClass);
        flags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse
                 | ImGuiWindowFlags_NoSavedSettings;
        name += kMaximizedIdSuffix;
    }
    const bool open = ImGui::Begin(name.c_str(), visibleFlag(), flags);

    // Сразу после Begin последний элемент - заголовок окна или его вкладка.
    bool toggle = ImGui::IsWindowDocked() && ImGui::IsItemHovered()
                  && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
    toggle = maximizeButton(isMaximized) || toggle;
    if (ImGui::BeginPopupContextItem(kPanelMenu)) {
        const char* key = isMaximized ? "editor.panel.restore" : "editor.panel.maximize";
        toggle = ImGui::MenuItem(tr(key).c_str(), "Shift+Space") || toggle;
        ImGui::EndPopup();
    }
    if (toggle && context.toggleMaximized) {
        context.toggleMaximized(*this);
    }
    m_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

    return open;
}

} // namespace yqlora::editor
