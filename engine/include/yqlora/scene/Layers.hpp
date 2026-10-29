#pragma once

#include <cstdint>

namespace yqlora {

// Слои узлов: маски камер и столкновения физики фильтруют по ним. Имена
// слоёв задаются в настройках проекта, в коде слой - номер.
inline constexpr int kLayerCount = 32;

// Набор слоёв: бит i - слой i.
using LayerMask = std::uint32_t;

inline constexpr LayerMask kAllLayers = ~LayerMask{0};

constexpr bool inMask(LayerMask mask, int layer)
{
    return ((mask >> layer) & 1u) != 0;
}

} // namespace yqlora
