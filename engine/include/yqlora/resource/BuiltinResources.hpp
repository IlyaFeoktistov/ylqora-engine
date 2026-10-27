#pragma once

#include "yqlora/resource/ResourceId.hpp"

namespace yqlora::builtin {

// Ресурсы, встроенные в движок. ID у них постоянные и маленькие,
// ID ресурсов проекта начинаются с kFirstProjectId и с ними не пересекаются.
inline constexpr ResourceId kCubeMesh = 1;   // ребро 1
inline constexpr ResourceId kSphereMesh = 2; // радиус 0.5
inline constexpr ResourceId kPlaneMesh = 3;  // 1 x 1 на плоскости XZ

inline constexpr ResourceId kFirstProjectId = 1024;

inline bool isBuiltin(ResourceId id)
{
    return id != 0 && id < kFirstProjectId;
}

} // namespace yqlora::builtin
