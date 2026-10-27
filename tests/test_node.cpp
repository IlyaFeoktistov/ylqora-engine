#include <doctest/doctest.h>

#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node3D.hpp>

using namespace yqlora;

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

TEST_CASE("Node: a removed child goes back to its place with insertChild")
{
    Node root;
    root.addChild<Node>("A");
    auto& b = root.addChild<Node>("B");
    root.addChild<Node>("C");

    auto owned = root.removeChild(b);
    root.insertChild(1, std::move(owned));
    REQUIRE(root.children().size() == 3);
    CHECK(root.children()[1].get() == &b);
    CHECK(b.parent() == &root);
}

TEST_CASE("Node3D: world transform follows the parent")
{
    Node3D parent;
    auto& child = parent.addChild<Node3D>();
    child.transform().setPosition({1.0f, 0.0f, 0.0f});

    parent.transform().setPosition({0.0f, 5.0f, 0.0f});
    CHECK(math::nearlyEqual(child.worldPosition(), {1.0f, 5.0f, 0.0f}));

    // Поворот родителя на 90 градусов вокруг Y переносит +X ребёнка в -Z.
    parent.transform().setRotationDegrees({0.0f, 90.0f, 0.0f});
    CHECK(math::nearlyEqual(child.worldPosition(), {0.0f, 5.0f, -1.0f}));
}

TEST_CASE("Node3D: reparenting invalidates cached world matrix")
{
    Node3D a;
    a.transform().setPosition({10.0f, 0.0f, 0.0f});
    Node3D b;

    auto& child = a.addChild<Node3D>();
    CHECK(math::nearlyEqual(child.worldPosition(), {10.0f, 0.0f, 0.0f}));

    auto owned = a.removeChild(child);
    b.addChild(std::move(owned));
    CHECK(math::nearlyEqual(child.worldPosition(), {0.0f, 0.0f, 0.0f}));
}

TEST_CASE("Node3D: default forward is -Z, lookAt turns towards target")
{
    Node3D node;
    CHECK(math::nearlyEqual(node.worldForward(), {0.0f, 0.0f, -1.0f}));

    node.transform().setPosition({0.0f, 0.0f, 0.0f});
    node.transform().lookAt({5.0f, 0.0f, 0.0f});
    CHECK(math::nearlyEqual(node.worldForward(), {1.0f, 0.0f, 0.0f}));
}

TEST_CASE("Node3D: setWorldPosition places a child in world space")
{
    Node3D parent;
    parent.transform().setPosition({10.0f, 0.0f, 0.0f});
    parent.transform().setRotationDegrees({0.0f, 90.0f, 0.0f});
    auto& child = parent.addChild<Node3D>();

    child.setWorldPosition({10.0f, 0.0f, -1.0f});
    CHECK(math::nearlyEqual(child.worldPosition(), {10.0f, 0.0f, -1.0f}));
    // В координатах повёрнутого родителя это точка на его оси +X.
    CHECK(math::nearlyEqual(child.transform().position(), {1.0f, 0.0f, 0.0f}));
}

TEST_CASE("Node3D: toWorld and toLocal are inverse to each other")
{
    Node3D node;
    node.transform().setPosition({0.0f, 5.0f, 0.0f});
    node.transform().setScale({2.0f, 2.0f, 2.0f});

    CHECK(math::nearlyEqual(node.toWorld({1.0f, 0.0f, 0.0f}), {2.0f, 5.0f, 0.0f}));
    CHECK(math::nearlyEqual(node.toLocal({2.0f, 5.0f, 0.0f}), {1.0f, 0.0f, 0.0f}));
}
