#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/math/Euler.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/TransformSystem3D.hpp>

#include <glm/gtc/matrix_transform.hpp>

using namespace yqlora;
using namespace yqlora::math;

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
    child.setWorldMatrix(world);

    // В координатах родителя со сдвигом 10 и масштабом 2.
    CHECK(near(child.transform().position(), {1.0f, 2.0f, 0.0f}));
    CHECK(near(child.transform().rotationDegrees(), {0.0f, 30.0f, 0.0f}));
    CHECK(near(child.transform().scale(), {2.0f, 2.0f, 2.0f}));
    CHECK(near(child.worldPosition(), {12.0f, 4.0f, 0.0f}));
}

TEST_CASE("World matrix: follows changes of the node, its parents and its parent")
{
    Node3D root;
    auto& middle = root.addChild<Node3D>();
    auto& leaf = middle.addChild<Node3D>();
    leaf.transform().setPosition({0.0f, 0.0f, 1.0f});
    CHECK(near(leaf.worldPosition(), {0.0f, 0.0f, 1.0f}));

    root.transform().setPosition({5.0f, 0.0f, 0.0f});
    CHECK(near(leaf.worldPosition(), {5.0f, 0.0f, 1.0f}));

    middle.transform().setScale({2.0f, 2.0f, 2.0f});
    CHECK(near(leaf.worldPosition(), {5.0f, 0.0f, 2.0f}));

    // Ветку целиком переносят под другого родителя.
    Node3D other;
    other.transform().setPosition({0.0f, 10.0f, 0.0f});
    other.addChild(root.removeChild(middle));
    CHECK(near(leaf.worldPosition(), {0.0f, 10.0f, 2.0f}));
}

TEST_CASE("setParent: KeepWorld decides whether the node stays in place")
{
    Scene scene{EngineServices::none()};
    auto& root = scene.createRoot<Node3D>();
    auto& hand = root.addChild<Node3D>();
    hand.transform().setPosition({10.0f, 0.0f, 0.0f});
    hand.transform().setRotationDegrees({0.0f, 30.0f, 0.0f});
    auto& sword = root.addChild<Node3D>();
    sword.transform().setPosition({1.0f, 2.0f, 3.0f});
    auto& wagon = root.addChild<Node3D>();
    wagon.transform().setPosition({1.0f, 2.0f, 3.0f});

    scene.setParent(sword, hand, KeepWorld::No);
    CHECK(near(sword.transform().position(), {1.0f, 2.0f, 3.0f}));
    CHECK_FALSE(near(sword.worldPosition(), {1.0f, 2.0f, 3.0f}));

    scene.setParent(wagon, hand, KeepWorld::Yes);
    CHECK(near(wagon.worldPosition(), {1.0f, 2.0f, 3.0f}));
    CHECK(near(wagon.transform().rotationDegrees(), {0.0f, -30.0f, 0.0f}));
}

TEST_CASE("TransformSystem3D: one pass refreshes the changed branches only")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    Input input;
    EngineServices services{input, registry};
    Scene scene{services};
    auto* system = scene.system<TransformSystem3D>();
    REQUIRE(system);

    auto& root = scene.createRoot<Node3D>();
    auto& child = root.addChild<Node3D>();
    scene.updateTransforms();
    CHECK(system->pendingCount() == 0);

    // Ветка встаёт в очередь один раз, сколько бы её ни меняли.
    root.transform().setPosition({1.0f, 0.0f, 0.0f});
    child.transform().setPosition({0.0f, 1.0f, 0.0f});
    root.transform().setScale({2.0f, 2.0f, 2.0f});
    CHECK(system->pendingCount() == 1);

    scene.updateTransforms();
    CHECK(system->pendingCount() == 0);
    CHECK(near(Vec3(child.worldMatrix()[3]), {1.0f, 2.0f, 0.0f}));

    // Ушедший из сцены узел из очереди убирается.
    child.transform().setPosition({0.0f, 2.0f, 0.0f});
    CHECK(system->pendingCount() == 1);
    auto removed = root.removeChild(child);
    CHECK(system->pendingCount() == 0);
}
