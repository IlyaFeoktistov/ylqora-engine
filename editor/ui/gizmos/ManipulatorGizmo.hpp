#pragma once

#include "Gizmo.hpp"

namespace yqlora {

// Манипуляторы выбранного узла, как в Unity: Q - выбор, W - перемещение,
// E - поворот, R - масштаб. Своя панель инструментов в левом верхнем углу
// окна сцены и переключатель осей: свои у объекта или мировые.
class ManipulatorGizmo final : public Gizmo {
public:
    enum class Tool {
        Select,
        Move,
        Rotate,
        Scale,
    };

    bool draw(GizmoContext& context) override;

private:
    // Возвращает true, если курсор над панелью.
    bool drawToolbar(const GizmoContext& context);
    void handleHotkeys();
    // Возвращает true, если курсор над манипулятором или его тянут.
    bool drawManipulator(GizmoContext& context);

    Tool m_tool = Tool::Move;
    bool m_localSpace = true;
};

} // namespace yqlora
