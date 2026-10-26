#include <doctest/doctest.h>

#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node3D.hpp>

#include <glm/gtc/epsilon.hpp>

using namespace yqlora;

namespace {

bool near(const Vec3& a, const Vec3& b)
{
    return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

} // namespace

TEST_CASE("Node: addChild sets parent and keeps order")
{
    Node root("Root");
    auto& a = root.addChild<Node3D>("A");
    auto& b = root.addChild<Camera3D>("B");

    CHECK(a.parent() == &root);
    CHECK(b.parent() == &root);
    REQUIRE(root.children().size() == 2);
    CHECK(root.children()[0].get() == &a);
    CHECK(root.findChild("B") == &b);
    CHECK(root.findChild("Missing") == nullptr);
}

TEST_CASE("Node: typeName is English and does not depend on the node name")
{
    Node3D empty("Пустышка");
    Camera3D camera;
    CHECK(empty.typeName() == "Node3D");
    CHECK(camera.typeName() == "Camera3D");
    CHECK(camera.name() == "Camera3D");
}

TEST_CASE("Node: every node gets a unique non-zero id")
{
    Node a;
    Node3D b;
    CHECK(a.id() != 0);
    CHECK(b.id() != 0);
    CHECK(a.id() != b.id());
}

TEST_CASE("Node: removeChild returns ownership")
{
    Node root;
    auto& child = root.addChild<Node>("Child");
    auto owned = root.removeChild(child);

    REQUIRE(owned);
    CHECK(owned->parent() == nullptr);
    CHECK(root.children().empty());

    Node other;
    CHECK(other.removeChild(*owned) == nullptr);
}

TEST_CASE("Node3D: world transform follows the parent")
{
    Node3D parent;
    auto& child = parent.addChild<Node3D>();
    child.transform().setPosition({1.0f, 0.0f, 0.0f});

    parent.transform().setPosition({0.0f, 5.0f, 0.0f});
    CHECK(near(child.worldPosition(), {1.0f, 5.0f, 0.0f}));

    // Поворот родителя на 90 градусов вокруг Y переносит +X ребёнка в -Z.
    parent.transform().setRotationDegrees({0.0f, 90.0f, 0.0f});
    CHECK(near(child.worldPosition(), {0.0f, 5.0f, -1.0f}));
}

TEST_CASE("Node3D: reparenting invalidates cached world matrix")
{
    Node3D a;
    a.transform().setPosition({10.0f, 0.0f, 0.0f});
    Node3D b;

    auto& child = a.addChild<Node3D>();
    CHECK(near(child.worldPosition(), {10.0f, 0.0f, 0.0f}));

    auto owned = a.removeChild(child);
    b.addChild(std::move(owned));
    CHECK(near(child.worldPosition(), {0.0f, 0.0f, 0.0f}));
}

TEST_CASE("Node3D: default forward is -Z, lookAt turns towards target")
{
    Node3D node;
    CHECK(near(node.forward(), {0.0f, 0.0f, -1.0f}));

    node.transform().setPosition({0.0f, 0.0f, 0.0f});
    node.transform().lookAt({5.0f, 0.0f, 0.0f});
    CHECK(near(node.forward(), {1.0f, 0.0f, 0.0f}));
}
