#pragma once

#include "yqlora/render/FrameData.hpp"

#include <imgui.h>

#include <memory>
#include <vector>

namespace yqlora::editor {

class EditorCamera;
class EditorSession;

// Всё, что нужно гизмо, чтобы нарисоваться поверх окна сцены.
struct GizmoContext {
    EditorSession& session;
    EditorCamera& camera;
    // Камера, через которую сейчас видна сцена.
    CameraData view;
    // Где картинка сцены на экране, в координатах интерфейса.
    ImVec2 origin;
    ImVec2 size;
    // Курсор над окном сцены, и его ещё не занял гизмо раньше в списке.
    bool mouseFree = false;
};

// Вспомогательная графика и инструменты редактора поверх сцены: сетка,
// контур выделения, камеры, оси, манипуляторы. В игре их нет.
class Gizmo {
public:
    virtual ~Gizmo() = default;

    // Что гизмо добавляет в сам кадр сцены (например, сетку или контур).
    virtual void prepareFrame(FrameData& /*frame*/, const EditorSession& /*session*/)
    {
    }

    // Рисует поверх кадра и обрабатывает мышь. Возвращает true, если курсор
    // занят этим гизмо: тогда гизмо дальше по списку клик не получают.
    virtual bool draw(GizmoContext& /*context*/)
    {
        return false;
    }
};

// Набор гизмо окна сцены. Порядок - это приоритет мыши: первый в списке
// получает клик первым. Слой включается и выключается целиком, например
// на время игры.
class GizmoLayer {
public:
    void add(std::unique_ptr<Gizmo> gizmo);

    void setEnabled(bool enabled)
    {
        m_enabled = enabled;
    }

    bool enabled() const
    {
        return m_enabled;
    }

    void prepareFrame(FrameData& frame, const EditorSession& session) const;

    // Возвращает true, если курсор занят каким-то гизмо.
    bool draw(GizmoContext& context);

private:
    std::vector<std::unique_ptr<Gizmo>> m_gizmos;
    bool m_enabled = true;
};

} // namespace yqlora::editor
