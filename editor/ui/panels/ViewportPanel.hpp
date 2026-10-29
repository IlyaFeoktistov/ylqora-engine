#pragma once

#include "core/EditorPanel.hpp"
#include "gizmos/Gizmo.hpp"

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/render/RenderTarget.hpp"

#include <imgui.h>

#include <memory>

namespace yqlora {

class Application;
class Camera3D;

} // namespace yqlora

namespace yqlora::editor {

struct EditorColors;

// Окно сцены: кадр рисуется в текстуру и показывается внутри окна интерфейса.
// Камера - редактора или, во время игры, главная камера сцены (см. RenderSystem). Всё остальное
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

    // Прямоугольник на экране.
    struct Rect {
        ImVec2 min;
        ImVec2 size;

        bool hovered() const;
    };

    // Подгоняет текстуру под окно и показывает её.
    ViewArea drawSceneImage(Application& app, const ImVec2& size);

    // Камера для превью: выбранная, если превью включено и смотрим через камеру редактора.
    const Camera3D* previewedCamera(EditorContext& context, bool throughGameCamera) const;
    // Угол окна сцены, где стоит превью: справа снизу, с соотношением сторон окна.
    static Rect previewRect(const ImVec2& origin, const ImVec2& size);
    // Кнопка превью справа сверху. Возвращает true, если курсор над ней.
    bool drawPreviewToggle(EditorContext& context, const ImVec2& origin, const ImVec2& size);
    // Картинка через камеру в своей текстуре, с рамкой и именем камеры.
    void drawCameraPreview(EditorContext& context, const Camera3D& camera, const Rect& rect);
    // Рисует кадр в текстуру: через камеру игры или редактора, с гизмо.
    void renderScene(EditorContext& context, float aspect, bool throughGameCamera, const GizmoContext& gizmoContext);
    void setFlying(bool flying);
    // Рамка вокруг окна, пока идёт игра, и подсказка, если в сцене нет камеры.
    void drawPlayModeFrame(const ImVec2& origin, const ImVec2& size, bool hasCamera, const EditorColors& colors);

    EditorCamera m_camera;
    GizmoLayer m_gizmos;
    // Курсор был над гизмо в прошлом кадре: правая кнопка над ним не запускает полёт.
    bool m_mouseOverGizmo = false;
    std::unique_ptr<RenderTarget> m_target;
    std::unique_ptr<RenderTarget> m_previewTarget;
};

} // namespace yqlora::editor
