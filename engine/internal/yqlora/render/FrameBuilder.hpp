#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/render/FrameData.hpp"

namespace yqlora {

class Scene;

// Снимок сцены для рендерера на текущий кадр.
YQLORA_API FrameData buildFrameData(const Scene& scene, float aspect, DepthRange depthRange);

} // namespace yqlora
