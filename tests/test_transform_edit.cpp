#include <doctest/doctest.h>

#include <yqlora/editor/TransformEdit.hpp>
#include <yqlora/scene/Node3D.hpp>

#include <glm/gtc/matrix_transform.hpp>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

// Углы после разложения матрицы теряют точность сильнее обычного.
constexpr float kAngleEpsilon = 1e-3f;

bool near(const Vec3& a, const Vec3& b)
{
    return math::nearlyEqual(a, b, kAngleEpsilon);
}

Quat fromDegrees(const Vec3& degrees)
{
    return Quat(glm::radians(degrees));
}

} // namespace

TEST_CASE("closestEulerDegrees: keeps the angles the inspector already shows")
{
    // Без подбора поворот на 100 градусов по Y вернулся бы как (180, 80, 180).
    CHECK(near(closestEulerDegrees(fromDegrees({0.0f, 100.0f, 0.0f}), {0.0f, 99.0f, 0.0f}), {0.0f, 100.0f, 0.0f}));

    // Крутили дальше 180: угол растёт, а не перескакивает на -179.
    CHECK(near(closestEulerDegrees(fromDegrees({0.0f, 0.0f, 181.0f}), {0.0f, 0.0f, 179.0f}), {0.0f, 0.0f, 181.0f}));

    // Несколько оборотов тоже сохраняются.
    CHECK(near(closestEulerDegrees(fromDegrees({0.0f, 0.0f, 725.0f}), {0.0f, 0.0f, 720.0f}), {0.0f, 0.0f, 725.0f}));
}

TEST_CASE("setWorldMatrix: world matrix becomes local position, rotation and scale")
{
    Node3D parent;
    parent.transform().setPosition({10.0f, 0.0f, 0.0f});
    parent.transform().setScale({2.0f, 2.0f, 2.0f});
    auto& child = parent.addChild<Node3D>();

    Mat4 world = glm::translate(Mat4(1.0f), Vec3(12.0f, 4.0f, 0.0f));
    world = glm::rotate(world, glm::radians(30.0f), Vec3(0.0f, 1.0f, 0.0f));
    world = glm::scale(world, Vec3(4.0f, 4.0f, 4.0f));
    setWorldMatrix(child, world);

    // В координатах родителя со сдвигом 10 и масштабом 2.
    CHECK(near(child.transform().position(), {1.0f, 2.0f, 0.0f}));
    CHECK(near(child.transform().rotationDegrees(), {0.0f, 30.0f, 0.0f}));
    CHECK(near(child.transform().scale(), {2.0f, 2.0f, 2.0f}));
    CHECK(near(child.worldPosition(), {12.0f, 4.0f, 0.0f}));
}
