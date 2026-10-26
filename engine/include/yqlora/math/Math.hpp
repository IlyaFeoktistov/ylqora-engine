#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace yqlora {

// Математика движка: алиасы над glm. Свои типы не пишем: glm проверен,
// совпадает с раскладкой данных в шейдерах и умеет SIMD. Алиасы оставляют
// возможность заменить библиотеку, не трогая код движка и игр.
using Vec2 = glm::vec2;
using Vec3 = glm::vec3;
using Vec4 = glm::vec4;
using Quat = glm::quat;
using Mat3 = glm::mat3;
using Mat4 = glm::mat4;

} // namespace yqlora
