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

} // namespace yqlora::editor
