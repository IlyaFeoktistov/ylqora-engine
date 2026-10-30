#include <doctest/doctest.h>

#include <yqlora/render/Camera3D.hpp>
#include <yqlora/scene/Node3D.hpp>

using namespace yqlora;

TEST_CASE("Transform3D: defaults to identity")
{
    const Node3D node;
    CHECK(math::nearlyEqual(node.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(math::nearlyEqual(node.transform().scale(), {1.0f, 1.0f, 1.0f}));
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
    CHECK(math::nearlyEqual(p, {0.0f, 1.0f, -2.0f}));
    CHECK(math::nearlyEqual(t.rotationDegrees(), {0.0f, 90.0f, 0.0f}));
}

TEST_CASE("Transform3D: Euler angles are returned exactly as set")
{
    Node3D node;
    auto& t = node.transform();

    // Через кватернион yaw 100 градусов вернулся бы как (180, 80, 180).
    t.setRotationDegrees({0.0f, 100.0f, 0.0f});
    CHECK(math::nearlyEqual(t.rotationDegrees(), {0.0f, 100.0f, 0.0f}));

    // После поворота кватернионом углы пересчитываются и описывают тот же поворот.
    t.setRotation(glm::angleAxis(glm::radians(30.0f), Vec3(1.0f, 0.0f, 0.0f)));
    CHECK(math::nearlyEqual(t.rotationDegrees(), {30.0f, 0.0f, 0.0f}));
}

TEST_CASE("Node3D: child transform is local to the parent")
{
    Node3D pivot("Pivot");
    pivot.transform().setPosition({10.0f, 30.0f, 0.0f});
    auto& camera = pivot.addChild<Camera3D>();

    // Камера стоит в начале координат родителя, а в мире там же, где он.
    CHECK(math::nearlyEqual(camera.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(math::nearlyEqual(camera.worldPosition(), {10.0f, 30.0f, 0.0f}));

    // Двигаем родителя, камера едет вместе с ним, её локальные координаты не меняются.
    pivot.transform().setPosition({0.0f, 5.0f, 0.0f});
    CHECK(math::nearlyEqual(camera.transform().position(), {0.0f, 0.0f, 0.0f}));
    CHECK(math::nearlyEqual(camera.worldPosition(), {0.0f, 5.0f, 0.0f}));

    // Двигаем камеру, смещение отсчитывается от родителя, сам он стоит на месте.
    camera.transform().setPosition({1.0f, 0.0f, 0.0f});
    CHECK(math::nearlyEqual(camera.worldPosition(), {1.0f, 5.0f, 0.0f}));
    CHECK(math::nearlyEqual(pivot.worldPosition(), {0.0f, 5.0f, 0.0f}));
}

TEST_CASE("Node3D: grandchild follows changes of the root")
{
    Node3D root;
    auto& child = root.addChild<Node3D>();
    auto& grandchild = child.addChild<Node3D>();
    grandchild.transform().setPosition({0.0f, 0.0f, 1.0f});
    CHECK(math::nearlyEqual(grandchild.worldPosition(), {0.0f, 0.0f, 1.0f}));

    root.transform().setPosition({0.0f, 10.0f, 0.0f});
    CHECK(math::nearlyEqual(grandchild.worldPosition(), {0.0f, 10.0f, 1.0f}));
}

TEST_CASE("Transform3D: values are read and written together")
{
    Node3D node;
    Transform3D& transform = node.transform();
    const TransformValues values{
        .position = {1.0f, 2.0f, 3.0f},
        .rotationDegrees = {0.0f, 45.0f, 0.0f},
        .scale = {2.0f, 2.0f, 2.0f},
    };
    transform.setValues(values);
    CHECK(transform.values() == values);
    CHECK(transform.position() == values.position);
}
