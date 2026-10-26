#include "CameraGizmo.hpp"

#include "Theme.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <array>
#include <cmath>
#include <optional>

namespace yqlora {

namespace {

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
        const ImU32 color = theme::toImU32(node.id() == selected ? colors.selectionOutline : colors.cameraGizmo);
        drawCamera(list, *camera, color, viewProjection, origin, size);
    }
    for (const auto& child : node.children()) {
        drawCamerasRecursive(list, *child, selected, viewProjection, origin, size);
    }
}

} // namespace

bool CameraGizmo::draw(GizmoContext& context)
{
    const Node* root = context.session.scene().root();
    if (!root) {
        return false;
    }
    const ImVec2& origin = context.origin;
    const ImVec2& size = context.size;
    ImDrawList& list = *ImGui::GetWindowDrawList();
    list.PushClipRect(origin, ImVec2(origin.x + size.x, origin.y + size.y), true);
    drawCamerasRecursive(list, *root, context.session.selection().selected(),
                         context.view.projection * context.view.view, origin, size);
    list.PopClipRect();

    return false;
}

} // namespace yqlora
