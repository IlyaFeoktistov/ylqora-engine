#pragma once

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/render/FrameData.hpp"
#include "yqlora/render/RenderTarget.hpp"

#include <imgui.h>

#include <memory>

namespace yqlora {

class Application;
class EditorSession;

// Окно сцены: кадр рисуется в текстуру и показывается внутри окна интерфейса.
// Над ним работают камера редактора, выбор объектов кликом и манипуляторы.
class ViewportPanel {
public:
    // Инструменты, как в Unity: Q, W, E, R.
    enum class Tool {
        Select,
        Move,
        Rotate,
        Scale,
    };

    void draw(Application& app, EditorSession& session, float deltaSeconds);

    EditorCamera& camera()
    {
        return m_camera;
    }

private:
    void setFlying(bool flying);
    // Рамка вокруг окна, пока идёт игра, и подсказка, если в сцене нет активной камеры.
    void drawPlayModeFrame(const ImVec2& origin, const ImVec2& size, bool hasCamera);
    // Панель инструментов в углу окна. Возвращает true, если курсор над ней.
    bool drawToolbar(const ImVec2& origin);
    void handleToolHotkeys();
    // Манипулятор выбранного узла. Возвращает true, если курсор над ним или его тянут.
    bool drawManipulator(EditorSession& session, const CameraData& view, const ImVec2& origin, const ImVec2& size);

    EditorCamera m_camera;
    Tool m_tool = Tool::Move;
    // Оси манипулятора: свои у объекта или мировые.
    bool m_localSpace = true;
    std::unique_ptr<RenderTarget> m_target;
};

} // namespace yqlora
