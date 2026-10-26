#pragma once

#include "yqlora/math/Ray.hpp"
#include "yqlora/scene/Node.hpp"

namespace yqlora {

class Scene;

// Ближайший к началу луча узел, в меш которого попадает луч, с точностью
// до треугольника. 0, если луч ни во что не попал.
NodeId pickNode(const Scene& scene, const Ray& ray);

} // namespace yqlora
