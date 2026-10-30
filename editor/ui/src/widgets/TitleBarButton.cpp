#include "yqlora/editor/ui/widgets/TitleBarButton.hpp"

#include <imgui.h>
#include <imgui_internal.h>

namespace yqlora::editor {

namespace {

constexpr const char* kButtonId = "#MAXIMIZE";
// Значок внутри кнопки: доля стороны кнопки, как у крестика ImGui.
constexpr float kGlyphShare = 0.5f;
// Второй квадрат значка "вернуть" сдвинут на эту долю значка.
constexpr float kRestoreOffsetShare = 0.3f;

// Квадрат "развернуть" или два квадрата "вернуть", линиями цвета текста.
void drawGlyph(ImDrawList& list, const ImRect& button, bool maximized)
{
    const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
    const float side = ImGui::GetFontSize() * kGlyphShare;
    const ImVec2 center = button.GetCenter();
    const ImVec2 min(center.x - side * 0.5f, center.y - side * 0.5f);
    const ImVec2 max(center.x + side * 0.5f, center.y + side * 0.5f);
    if (!maximized) {
        list.AddRect(min, max, color);

        return;
    }
    const float offset = side * kRestoreOffsetShare;
    list.AddRect(ImVec2(min.x + offset, min.y - offset), ImVec2(max.x + offset, max.y - offset), color);
    list.AddRectFilled(min, max, ImGui::GetColorU32(ImGuiCol_TitleBgActive));
    list.AddRect(min, max, color);
}

} // namespace

bool maximizeButton(bool maximized)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    const ImGuiStyle& style = ImGui::GetStyle();
    const float size = ImGui::GetFontSize();

    // Где заголовок и чей он: у пристыкованного окна полосу вкладок рисует
    // окно-хозяин узла, туда и кнопку.
    ImGuiWindow* owner = window;
    ImRect bar;
    bool closeButton = false;
    if (window->DockIsActive) {
        ImGuiDockNode* node = window->DockNode;
        if (!node || node->VisibleWindow != window || node->IsHiddenTabBar() || node->IsNoTabBar()) {
            return false;
        }
        owner = node->HostWindow;
        bar = ImRect(node->Pos, ImVec2(node->Pos.x + node->Size.x, node->Pos.y + ImGui::GetFrameHeight()));
        closeButton = node->HasCloseButton;
    } else {
        if ((window->Flags & ImGuiWindowFlags_NoTitleBar) != 0) {
            return false;
        }
        bar = window->TitleBarRect();
        closeButton = window->HasCloseButton;
    }
    if (!owner) {
        return false;
    }

    // Так же, как ImGui ставит крестик: от правого края с отступом рамки.
    float right = bar.Max.x - style.FramePadding.x;
    if (closeButton) {
        right -= size + style.ItemInnerSpacing.x;
    }
    const ImVec2 min(right - size, bar.Min.y + style.FramePadding.y);
    const ImRect button(min, ImVec2(min.x + size, min.y + size));

    // Кнопка - элемент окна, под которым её видно мыши: заголовок у окна
    // свой, полоса вкладок - у хозяина. Хозяин дописывается вторым Begin.
    const bool foreign = owner != window;
    if (foreign) {
        ImGui::Begin(owner->Name);
    }
    ImGui::PushID(static_cast<int>(window->ID));
    const ImGuiID id = ImGui::GetID(kButtonId);
    ImGui::PopID();
    ImGui::PushClipRect(bar.Min, bar.Max, false);
    bool pressed = false;
    if (ImGui::ItemAdd(button, id)) {
        bool hovered = false;
        bool held = false;
        pressed = ImGui::ButtonBehavior(button, id, &hovered, &held);
        ImDrawList& list = *ImGui::GetWindowDrawList();
        if (hovered) {
            list.AddRectFilled(
                button.Min,
                button.Max,
                ImGui::GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered)
            );
        }
        drawGlyph(list, button, maximized);
    }
    ImGui::PopClipRect();
    if (foreign) {
        ImGui::End();
    }

    return pressed;
}

} // namespace yqlora::editor
