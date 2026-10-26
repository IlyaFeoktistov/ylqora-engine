#pragma once

#include "gizmos/Gizmo.hpp"

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/render/RenderTarget.hpp"

#include <imgui.h>

#include <memory>

namespace yqlora {

class Application;
class EditorSession;

// Окно сцены: кадр рисуется в текстуру и показывается внутри окна интерфейса.
// Камера - редактора или, во время игры, активная камера сцены. Всё остальное
// поверх сцены (сетка, выделение, манипуляторы) делают гизмо, и в игре их нет.
class ViewportPanel {
public:
    ViewportPanel();

    void draw(Application& app, EditorSession& session, float deltaSeconds);

    EditorCamera& camera()
    {
        return m_camera;
    }

private:
    void setFlying(bool flying);
    // Рамка вокруг окна, пока идёт игра, и подсказка, если в сцене нет активной камеры.
    void drawPlayModeFrame(const ImVec2& origin, const ImVec2& size, bool hasCamera);

    EditorCamera m_camera;
    GizmoLayer m_gizmos;
    // Курсор был над гизмо в прошлом кадре: правая кнопка над ним не запускает полёт.
    bool m_mouseOverGizmo = false;
    std::unique_ptr<RenderTarget> m_target;
};

} // namespace yqlora
