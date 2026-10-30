#include "yqlora/editor/ui/gizmos/AxisGizmo.hpp"

#include "yqlora/editor/ui/style/Theme.hpp"

#include "yqlora/editor/EditorCamera.hpp"

#include <algorithm>
#include <array>

namespace yqlora::editor {

bool AxisGizmo::draw(GizmoContext& context)
{
    const Mat4& view = context.view.view;
    const ImVec2& origin = context.origin;
    const ImVec2& size = context.size;

    constexpr float kRadius = 38.0f; // длина оси на экране
    constexpr float kMargin = 14.0f;
    constexpr float kBallRadius = 9.0f;
    constexpr int kAxisCount = 3;
    constexpr float kBackgroundPadding = 4.0f;
    constexpr int kBackgroundAlpha = 30;
    constexpr int kBackgroundHoveredAlpha = 60;
    constexpr float kAxisThickness = 2.0f;
    // Отрицательные концы осей бледнее и меньше, чтобы не путать их с положительными.
    constexpr float kPositiveAlpha = 0.9f;
    constexpr float kNegativeAlpha = 0.45f;
    constexpr float kNegativeBallScale = 0.75f;

    const EditorColors& colors = context.theme.colors();
    const Vec4 axisColors[] = {colors.axisX, colors.axisY, colors.axisZ};
    const char* names[] = {"X", "Y", "Z"};

    const ImVec2 center(
        origin.x + size.x - kRadius - kBallRadius - kMargin,
        origin.y + kRadius + kBallRadius + kMargin
    );
    const Mat3 rotation(view);

    struct End {
        ImVec2 position;
        float depth;
        int axis;
        bool positive;
    };
    // У каждой оси два конца: положительный и отрицательный.
    std::array<End, kAxisCount * 2> ends{};
    for (int axis = 0; axis < kAxisCount; ++axis) {
        Vec3 direction(0.0f);
        direction[axis] = 1.0f;
        // Направление оси в координатах камеры: x вправо, y вверх, z на нас.
        const Vec3 v = rotation * direction;
        const ImVec2 offset(v.x * kRadius, -v.y * kRadius);
        ends[static_cast<std::size_t>(axis * 2)] = {ImVec2(center.x + offset.x, center.y + offset.y), v.z, axis, true};
        ends[static_cast<std::size_t>(axis * 2 + 1)] =
            {ImVec2(center.x - offset.x, center.y - offset.y), -v.z, axis, false};
    }
    // Сначала дальние, потом ближние: ближние перекрывают дальние.
    std::ranges::sort(ends, [](const End& a, const End& b) {
        return a.depth < b.depth;
    });

    // Какой кружок под курсором: ближний из тех, что под ним.
    const ImVec2 mouse = ImGui::GetMousePos();
    const End* hovered = nullptr;
    const bool mouseOverViewport = context.mouseFree;
    for (const End& end : ends) {
        const float dx = mouse.x - end.position.x;
        const float dy = mouse.y - end.position.y;
        if (mouseOverViewport && dx * dx + dy * dy <= kBallRadius * kBallRadius) {
            hovered = &end;
        }
    }

    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.AddCircleFilled(
        center,
        kRadius + kBallRadius + kBackgroundPadding,
        IM_COL32(0, 0, 0, hovered ? kBackgroundHoveredAlpha : kBackgroundAlpha)
    );
    for (const End& end : ends) {
        const Vec4& color = axisColors[end.axis];
        const float alpha = &end == hovered ? 1.0f : end.positive ? kPositiveAlpha : kNegativeAlpha;
        if (end.positive) {
            list.AddLine(center, end.position, toImU32(color, alpha), kAxisThickness);
        }
        list.AddCircleFilled(
            end.position,
            end.positive ? kBallRadius : kBallRadius * kNegativeBallScale,
            toImU32(color, alpha)
        );
        if (end.positive) {
            const ImVec2 textSize = ImGui::CalcTextSize(names[end.axis]);
            list.AddText(
                ImVec2(end.position.x - textSize.x * 0.5f, end.position.y - textSize.y * 0.5f),
                IM_COL32(20, 20, 20, 255),
                names[end.axis]
            );
        }
    }

    if (!hovered) {
        return false;
    }
    ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        // Клик по +X: смотрим со стороны +X на начало координат, то есть вдоль -X.
        Vec3 direction(0.0f);
        direction[hovered->axis] = hovered->positive ? -1.0f : 1.0f;
        context.camera.lookAlong(direction);
    }

    return true;
}

} // namespace yqlora::editor
