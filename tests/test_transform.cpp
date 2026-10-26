#include <doctest/doctest.h>

#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node2D.hpp>
#include <yqlora/scene/Node3D.hpp>

#include <glm/gtc/epsilon.hpp>

using namespace yqlora;

namespace {

bool near(const glm::vec3& a, const glm::vec3& b)
{
    return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

bool near(const glm::vec2& a, const glm::vec2& b)
{
    return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

} // namespace

TEST_CASE("Transform3D: defaults to identity")
{
    const Node3D node;
    CHECK(near(node.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(near(node.transform().scale(), {1.0f, 1.0f, 1.0f}));
    CHECK(node.transform().matrix() == glm::mat4(1.0f));
}

TEST_CASE("Transform3D: matrix applies scale, then rotation, then translation")
{
    Node3D node;
    auto& t = node.transform();
    t.setScale({2.0f, 2.0f, 2.0f});
    t.setRotationDegrees({0.0f, 90.0f, 0.0f});
    t.setPosition({0.0f, 1.0f, 0.0f});

    const glm::vec3 p = glm::vec3(t.matrix() * glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
    CHECK(near(p, {0.0f, 1.0f, -2.0f}));
    CHECK(near(t.rotationDegrees(), {0.0f, 90.0f, 0.0f}));
}

TEST_CASE("Transform3D: every change bumps the version")
{
    Node3D node;
    const auto v0 = node.transform().version();
    node.transform().setPosition({1.0f, 0.0f, 0.0f});
    CHECK(node.transform().version() != v0);
}

TEST_CASE("Node3D: child transform is local to the parent")
{
    Node3D pivot("Pivot");
    pivot.transform().setPosition({10.0f, 30.0f, 0.0f});
    auto& camera = pivot.addChild<Camera3D>();

    // Камера стоит в начале координат родителя, а в мире — там же, где он.
    CHECK(near(camera.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(near(camera.worldPosition(), {10.0f, 30.0f, 0.0f}));

    // Двигаем родителя — камера едет вместе с ним, её локальные координаты не меняются.
    pivot.transform().setPosition({0.0f, 5.0f, 0.0f});
    CHECK(near(camera.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(near(camera.worldPosition(), {0.0f, 5.0f, 0.0f}));

    // Двигаем камеру — смещение отсчитывается от родителя, сам он стоит на месте.
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

    // +X после масштаба 2 и поворота на 90° против часовой смотрит в +Y.
    const glm::vec2 p = glm::vec2(t.matrix() * glm::vec3(1.0f, 0.0f, 1.0f));
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
