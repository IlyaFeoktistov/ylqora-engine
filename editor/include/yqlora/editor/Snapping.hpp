#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/math/Ray.hpp"

namespace yqlora {
class Scene;
} // namespace yqlora

namespace yqlora::editor {

// Привязка при перемещении в окне вида (см. docs/architecture/editor.md).
// Привычка человека, поэтому в настройках редактора, а не проекта.
struct SnapSettings {
    static constexpr float kDefaultMoveStep = 1.0f;
    static constexpr float kDefaultRotateStepDegrees = 15.0f;
    static constexpr float kDefaultScaleStep = 0.1f;

    // Кнопка "по сетке" на панели окна вида; Ctrl включает на одно перетаскивание.
    bool grid = false;
    // Шаг перемещения в метрах: положение в мире кратно ему.
    float moveStep = kDefaultMoveStep;
    float rotateStepDegrees = kDefaultRotateStepDegrees;
    float scaleStep = kDefaultScaleStep;
    // При привязке к поверхности ещё и повернуть узел по её нормали.
    bool alignToSurface = false;

    bool operator==(const SnapSettings&) const = default;
};

// Шаги перемещения в списке на панели; свой шаг задаётся там же числом.
inline constexpr float kMoveStepChoices[] = {1.0f, 0.5f, 0.25f, 0.1f};

// Ближайшая точка сетки с шагом step; при step <= 0 точка не меняется.
Vec3 snapToGrid(const Vec3& point, float step);

// Куда встаёт ресурс, отпущенный в окне вида под лучом ray: на поверхность
// меша, если луч в неё попал, иначе на землю (placementPoint); при
// включённой сетке - в ближайшую её точку.
Vec3 dropPoint(const Scene& scene, const Ray& ray, const SnapSettings& snap);

// Поворот rotation, у которого локальная ось Y смотрит вдоль normal: узел
// встаёт на поверхность "ногами", кратчайшим доворотом.
Quat alignUpTo(const Quat& rotation, const Vec3& normal);

} // namespace yqlora::editor
