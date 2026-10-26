#include <doctest/doctest.h>

#include <yqlora/render/FrameBuilder.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Scene.hpp>

using namespace yqlora;

TEST_CASE("Scene: new scene is empty")
{
    Scene scene;
    CHECK(scene.empty());
    CHECK(scene.root() == nullptr);

    // Пустую сцену можно обновлять и рисовать.
    scene.update(0.016f);
    const FrameData frame = buildFrameData(scene, 1.0f, DepthRange::MinusOneToOne);
    CHECK(frame.clearColor == scene.clearColor());
}

TEST_CASE("Scene: root can be of any node type")
{
    Scene scene;
    auto& root = scene.createRoot<Node3D>("World");
    CHECK_FALSE(scene.empty());
    CHECK(scene.root() == &root);
    CHECK(root.parent() == nullptr);
}

TEST_CASE("Scene: replacing the root resets the active camera")
{
    Scene scene;
    auto& camera = scene.createRoot<Camera3D>();
    scene.setActiveCamera(&camera);

    scene.createRoot<Node>();
    CHECK(scene.activeCamera() == nullptr);
}

TEST_CASE("Scene: takeRoot leaves the scene empty")
{
    Scene scene;
    auto& root = scene.createRoot<Node>();

    auto owned = scene.takeRoot();
    CHECK(owned.get() == &root);
    CHECK(scene.empty());
}
