#include <doctest/doctest.h>

#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node2D.hpp>
#include <yqlora/scene/Node3D.hpp>

#include <glm/gtc/epsilon.hpp>

using namespace yqlora;

namespace {

bool near(const Vec3& a, const Vec3& b)
{
    return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

bool near(const Vec2& a, const Vec2& b)
{
    return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

} // namespace

TEST_CASE("Transform3D: defaults to identity")
{
    const Node3D node;
    CHECK(near(node.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(near(node.transform().scale(), {1.0f, 1.0f, 1.0f}));
    CHECK(node.transform().matrix() == Mat4(1.0f));
}

TEST_CASE("Transform3D: matrix applies scale, then rotation, then translation")
{
    Node3D node;
    auto& t = node.transform();
    t.setScale({2.0f, 2.0f, 2.0f});
    t.setRotationDegrees({0.0f, 90.0f, 0.0f});
    t.setPosition({0.0f, 1.0f, 0.0f});

    const Vec3 p = Vec3(t.matrix() * Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    CHECK(near(p, {0.0f, 1.0f, -2.0f}));
    CHECK(near(t.rotationDegrees(), {0.0f, 90.0f, 0.0f}));
}

TEST_CASE("Transform3D: Euler angles are returned exactly as set")
{
    Node3D node;
    auto& t = node.transform();

    // Через кватернион yaw 100 градусов вернулся бы как (180, 80, 180).
    t.setRotationDegrees({0.0f, 100.0f, 0.0f});
    CHECK(near(t.rotationDegrees(), {0.0f, 100.0f, 0.0f}));

    // После поворота кватернионом углы пересчитываются и описывают тот же поворот.
    t.setRotation(glm::angleAxis(glm::radians(30.0f), Vec3(1.0f, 0.0f, 0.0f)));
    CHECK(near(t.rotationDegrees(), {30.0f, 0.0f, 0.0f}));
}

TEST_CASE("Node3D: child transform is local to the parent")
{
    Node3D pivot("Pivot");
    pivot.transform().setPosition({10.0f, 30.0f, 0.0f});
    auto& camera = pivot.addChild<Camera3D>();

    // Камера стоит в начале координат родителя, а в мире там же, где он.
    CHECK(near(camera.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(near(camera.worldPosition(), {10.0f, 30.0f, 0.0f}));

    // Двигаем родителя, камера едет вместе с ним, её локальные координаты не меняются.
    pivot.transform().setPosition({0.0f, 5.0f, 0.0f});
    CHECK(near(camera.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(near(camera.worldPosition(), {0.0f, 5.0f, 0.0f}));

    // Двигаем камеру, смещение отсчитывается от родителя, сам он стоит на месте.
    camera.transform().setPosition({1.0f, 0.0f, 0.0f});
    CHECK(near(camera.worldPosition(), {1.0f, 5.0f, 0.0f}));
    CHECK(near(pivot.worldPosition(), {0.0f, 5.0f, 0.0f}));
}

TEST_CASE("Node3D: grandchild follows changes of the root")
{
    Node3D root;
    auto& child = root.addChild<Node3D>();
    auto& grandchild = child.addChild<Node3D>();
    grandchild.transform().setPosition({0.0f, 0.0f, 1.0f});
    CHECK(near(grandchild.worldPosition(), {0.0f, 0.0f, 1.0f}));

    root.transform().setPosition({0.0f, 10.0f, 0.0f});
    CHECK(near(grandchild.worldPosition(), {0.0f, 10.0f, 1.0f}));
}

TEST_CASE("Transform2D: matrix applies scale, then rotation, then translation")
{
    Node2D node;
    auto& t = node.transform();
    t.setScale({2.0f, 2.0f});
    t.setRotationDegrees(90.0f);
    t.setPosition({0.0f, 1.0f});

    // +X после масштаба 2 и поворота на 90 градусов против часовой смотрит в +Y.
    const Vec2 p = Vec2(t.matrix() * Vec3(1.0f, 0.0f, 1.0f));
    CHECK(near(p, {0.0f, 3.0f}));
    CHECK(t.rotationDegrees() == doctest::Approx(90.0f));
}

TEST_CASE("Node2D: world position follows the parent")
{
    Node2D parent;
    auto& child = parent.addChild<Node2D>();
    child.transform().setPosition({1.0f, 0.0f});

    parent.transform().setPosition({0.0f, 5.0f});
    CHECK(near(child.worldPosition(), {1.0f, 5.0f}));

    parent.transform().setRotationDegrees(90.0f);
    CHECK(near(child.worldPosition(), {0.0f, 6.0f}));
}

TEST_CASE("Node2D: reparenting into a 3D branch makes the node a world root")
{
    Node2D parent;
    parent.transform().setPosition({3.0f, 0.0f});
    auto& child = parent.addChild<Node2D>();
    CHECK(near(child.worldPosition(), {3.0f, 0.0f}));

    Node3D other;
    other.addChild(parent.removeChild(child));
    CHECK(near(child.worldPosition(), {0.0f, 0.0f}));
}
