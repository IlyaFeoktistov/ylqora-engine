#pragma once

#include "core/EditorPanel.hpp"
#include "gizmos/Gizmo.hpp"

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/render/RenderTarget.hpp"

#include <imgui.h>

#include <memory>

namespace yqlora {

class Application;

} // namespace yqlora

namespace yqlora::editor {

struct EditorColors;

// Окно сцены: кадр рисуется в текстуру и показывается внутри окна интерфейса.
// Камера - редактора или, во время игры, активная камера сцены. Всё остальное
// поверх сцены (сетка, выделение, манипуляторы) делают гизмо, и в игре их нет.
class ViewportPanel final : public EditorPanel {
public:
    ViewportPanel();

    const char* id() const override
    {
        return "Viewport";
    }

    const char* titleKey() const override
    {
        return "editor.window.viewport";
    }

    bool closable() const override
    {
        return false;
    }

    void draw(EditorContext& context) override;
    void onProjectOpened(EditorContext& context) override;

private:
    // Где на экране кадр сцены и наведена ли на него мышь.
    struct ViewArea {
        ImVec2 origin;
        bool hovered = false;
    };

    // Подгоняет текстуру под окно и показывает её.
    ViewArea drawSceneImage(Application& app, const ImVec2& size);
    // Рисует кадр в текстуру: через камеру игры или редактора, с гизмо.
    void renderScene(EditorContext& context, float aspect, bool throughGameCamera, const GizmoContext& gizmoContext);
    void setFlying(bool flying);
    // Рамка вокруг окна, пока идёт игра, и подсказка, если в сцене нет активной камеры.
    void drawPlayModeFrame(const ImVec2& origin, const ImVec2& size, bool hasCamera, const EditorColors& colors);

    EditorCamera m_camera;
    GizmoLayer m_gizmos;
    // Курсор был над гизмо в прошлом кадре: правая кнопка над ним не запускает полёт.
    bool m_mouseOverGizmo = false;
    std::unique_ptr<RenderTarget> m_target;
};

} // namespace yqlora::editor
