#pragma once

#include "yqlora/render/FrameData.hpp"

namespace yqlora {

class Scene;

// Снимок сцены для рендерера на текущий кадр.
FrameData buildFrameData(const Scene& scene, float aspect, DepthRange depthRange);

} // namespace yqlora
