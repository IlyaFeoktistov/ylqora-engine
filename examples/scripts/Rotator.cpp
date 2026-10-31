// Пример модуля игры: скрипт, который вращает свой узел. Показывает поля с
// атрибутами, правило узла и входную функцию модуля.

#include <yqlora/math/Math.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Transform3D.hpp>
#include <yqlora/scripting/ModuleRegistrar.hpp>
#include <yqlora/scripting/Script.hpp>

#include <glm/gtc/quaternion.hpp>

namespace {

constexpr float kMaxDegreesPerSecond = 720.0f;

class Rotator : public yqlora::Script {
public:
    YQLORA_SCRIPT(Rotator)

    float degreesPerSecond = 90.0f;
    // Ось в пространстве узла; длина не важна, нулевая - вращения нет.
    yqlora::Vec3 axis = yqlora::math::kWorldUp;

    YQLORA_FIELDS(
        field("degreesPerSecond", &Rotator::degreesPerSecond)
            .range(-kMaxDegreesPerSecond, kMaxDegreesPerSecond)
            .tooltip("Turn speed, degrees per second; negative turns the other way"),
        field("axis", &Rotator::axis).tooltip("Axis in the space of the node")
    )

protected:
    void onUpdate(float deltaSeconds) override
    {
        if (glm::length(axis) == 0.0f) {
            return;
        }
        const yqlora::Quat turn = glm::angleAxis(glm::radians(degreesPerSecond * deltaSeconds), glm::normalize(axis));
        transform().setRotation(transform().rotation() * turn);
    }
};

} // namespace

YQLORA_MODULE_ENTRY(registrar)
{
    registrar.registerScript<Rotator>().requiresNode<yqlora::Node3D>();
}
