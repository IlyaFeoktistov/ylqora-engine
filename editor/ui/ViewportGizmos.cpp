#include "ViewportGizmos.hpp"

#include "Theme.hpp"

#include "yqlora/scene/Camera3D.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace yqlora::gizmos {

namespace {

ImU32 toColor(const Vec4& color, float alpha = 1.0f)
{
    return ImGui::GetColorU32(ImVec4(color.r, color.g, color.b, color.a * alpha));
}

// Точка мира на экране. nullopt, если она за камерой: такой отрезок не рисуем.
std::optional<ImVec2> project(const Mat4& viewProjection, const Vec3& point, const ImVec2& origin, const ImVec2& size)
{
    const Vec4 clip = viewProjection * Vec4(point, 1.0f);
    if (clip.w <= 0.01f) {
        return std::nullopt;
    }
    const Vec2 ndc = Vec2(clip.x, clip.y) / clip.w;

    return ImVec2(origin.x + (ndc.x * 0.5f + 0.5f) * size.x, origin.y + (0.5f - ndc.y * 0.5f) * size.y);
}

void line(ImDrawList& list, const Mat4& viewProjection, const Vec3& a, const Vec3& b, const ImVec2& origin,
          const ImVec2& size, ImU32 color)
{
    const auto screenA = project(viewProjection, a, origin, size);
    const auto screenB = project(viewProjection, b, origin, size);
    if (screenA && screenB) {
        list.AddLine(*screenA, *screenB, color, 1.5f);
    }
}

void drawCamera(ImDrawList& list, const Camera3D& camera, ImU32 color, const Mat4& viewProjection,
                const ImVec2& origin, const ImVec2& size)
{
    // Пирамида глубиной в полметра, пропорции экрана 16:9.
    constexpr float kDepth = 0.5f;
    constexpr float kAspect = 16.0f / 9.0f;
    const float halfHeight = kDepth * std::tan(glm::radians(camera.fovDegrees()) * 0.5f);
    const float halfWidth = halfHeight * kAspect;

    const Mat4& world = camera.worldMatrix();
    const auto toWorld = [&](float x, float y, float z) {
        return Vec3(world * Vec4(x, y, z, 1.0f));
    };
    const Vec3 apex = toWorld(0.0f, 0.0f, 0.0f);
    const std::array<Vec3, 4> corners = {
        toWorld(-halfWidth, -halfHeight, -kDepth),
        toWorld(halfWidth, -halfHeight, -kDepth),
        toWorld(halfWidth, halfHeight, -kDepth),
        toWorld(-halfWidth, halfHeight, -kDepth),
    };

    for (std::size_t i = 0; i < corners.size(); ++i) {
        line(list, viewProjection, apex, corners[i], origin, size, color);
        line(list, viewProjection, corners[i], corners[(i + 1) % corners.size()], origin, size, color);
    }
    // Треугольник над верхней гранью показывает, где у камеры верх.
    const Vec3 left = toWorld(-halfWidth * 0.5f, halfHeight * 1.1f, -kDepth);
    const Vec3 right = toWorld(halfWidth * 0.5f, halfHeight * 1.1f, -kDepth);
    const Vec3 top = toWorld(0.0f, halfHeight * 1.6f, -kDepth);
    line(list, viewProjection, left, right, origin, size, color);
    line(list, viewProjection, right, top, origin, size, color);
    line(list, viewProjection, top, left, origin, size, color);
}

void drawCamerasRecursive(ImDrawList& list, const Node& node, NodeId selected, const Mat4& viewProjection,
                          const ImVec2& origin, const ImVec2& size)
{
    if (const auto* camera = dynamic_cast<const Camera3D*>(&node)) {
        const EditorColors& colors = theme::colors();
        const ImU32 color = toColor(node.id() == selected ? colors.selectionOutline : colors.cameraGizmo);
        drawCamera(list, *camera, color, viewProjection, origin, size);
    }
    for (const auto& child : node.children()) {
        drawCamerasRecursive(list, *child, selected, viewProjection, origin, size);
    }
}

} // namespace

void drawCameras(const Node& root, NodeId selected, const CameraData& view, const ImVec2& origin, const ImVec2& size)
{
    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    drawCamerasRecursive(list, root, selected, view.projection * view.view, origin, size);
    list.PopClipRect();
}

AxisGizmoResult drawAxes(const Mat4& view, const ImVec2& origin, const ImVec2& size)
{
    constexpr float kRadius = 38.0f; // длина оси на экране
    constexpr float kMargin = 14.0f;
    constexpr float kBallRadius = 9.0f;

    const EditorColors& colors = theme::colors();
    const Vec4 axisColors[] = {colors.axisX, colors.axisY, colors.axisZ};
    const char* names[] = {"X", "Y", "Z"};

    const ImVec2 center(origin.x + size.x - kRadius - kBallRadius - kMargin, origin.y + kRadius + kBallRadius + kMargin);
    const Mat3 rotation(view);

    struct End {
        ImVec2 position;
        float depth;
        int axis;
        bool positive;
    };
    std::array<End, 6> ends{};
    for (int axis = 0; axis < 3; ++axis) {
        Vec3 direction(0.0f);
        direction[axis] = 1.0f;
        // Направление оси в координатах камеры: x вправо, y вверх, z на нас.
        const Vec3 v = rotation * direction;
        const ImVec2 offset(v.x * kRadius, -v.y * kRadius);
        ends[static_cast<std::size_t>(axis * 2)] = {ImVec2(center.x + offset.x, center.y + offset.y), v.z, axis, true};
        ends[static_cast<std::size_t>(axis * 2 + 1)] = {ImVec2(center.x - offset.x, center.y - offset.y), -v.z, axis,
                                                        false};
    }
    // Сначала дальние, потом ближние: ближние перекрывают дальние.
    std::ranges::sort(ends, [](const End& a, const End& b) {
        return a.depth < b.depth;
    });

    // Какой кружок под курсором: ближний из тех, что под ним.
    const ImVec2 mouse = ImGui::GetMousePos();
    const End* hovered = nullptr;
    const bool mouseOverViewport = ImGui::IsWindowHovered();
    for (const End& end : ends) {
        const float dx = mouse.x - end.position.x;
        const float dy = mouse.y - end.position.y;
        if (mouseOverViewport && dx * dx + dy * dy <= kBallRadius * kBallRadius) {
            hovered = &end;
        }
    }

    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.AddCircleFilled(center, kRadius + kBallRadius + 4.0f, IM_COL32(0, 0, 0, hovered ? 60 : 30));
    for (const End& end : ends) {
        const Vec4& color = axisColors[end.axis];
        const float alpha = &end == hovered ? 1.0f : end.positive ? 0.9f : 0.45f;
        if (end.positive) {
            list.AddLine(center, end.position, toColor(color, alpha), 2.0f);
        }
        list.AddCircleFilled(end.position, end.positive ? kBallRadius : kBallRadius * 0.75f, toColor(color, alpha));
        if (end.positive) {
            const ImVec2 textSize = ImGui::CalcTextSize(names[end.axis]);
            list.AddText(ImVec2(end.position.x - textSize.x * 0.5f, end.position.y - textSize.y * 0.5f),
                         IM_COL32(20, 20, 20, 255), names[end.axis]);
        }
    }

    AxisGizmoResult result;
    result.hovered = hovered != nullptr;
    if (hovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            // Клик по +X: смотрим со стороны +X на начало координат, то есть вдоль -X.
            Vec3 direction(0.0f);
            direction[hovered->axis] = hovered->positive ? -1.0f : 1.0f;
            result.lookAlong = direction;
        }
    }

    return result;
}

} // namespace yqlora::gizmos
