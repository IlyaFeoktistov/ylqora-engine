#pragma once

#include "yqlora/editor/EditorCamera.hpp"
#include "yqlora/render/RenderTarget.hpp"

#include <memory>

namespace yqlora {

class Application;
class EditorSession;

// Окно сцены: кадр рисуется в текстуру и показывается внутри окна интерфейса.
// Над ним работают камера редактора и выбор объектов кликом.
class ViewportPanel {
public:
    void draw(Application& app, EditorSession& session, float deltaSeconds);

    EditorCamera& camera()
    {
        return m_camera;
    }

private:
    void setFlying(Application& app, bool flying);

    EditorCamera m_camera;
    std::unique_ptr<RenderTarget> m_target;
};

} // namespace yqlora
