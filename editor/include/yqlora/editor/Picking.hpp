#pragma once

#include "yqlora/math/Ray.hpp"
#include "yqlora/scene/Node.hpp"

namespace yqlora {

class Scene;

} // namespace yqlora

namespace yqlora::editor {

// Своей геометрии у камеры нет, поэтому её выбирают по сфере такого радиуса.
inline constexpr float kCameraPickRadius = 0.3f;

// Ближайший к началу луча узел, в который попадает луч: меш - с точностью
// до треугольника, камера - по сфере вокруг неё. 0, если луч ни во что не попал.
NodeId pickNode(const Scene& scene, const Ray& ray);

// Куда поставить ресурс, отпущенный в окне вида под лучом ray: точка на
// плоскости земли (y = 0), если луч в неё смотрит не дальше
// kMaxPlacementDistance, иначе точка луча на kPlacementDistance. Привязка к
// сетке и к поверхностям - отдельно, см. editor.md.
inline constexpr float kPlacementDistance = 10.0f;
inline constexpr float kMaxPlacementDistance = 100.0f;
Vec3 placementPoint(const Ray& ray);

} // namespace yqlora::editor
