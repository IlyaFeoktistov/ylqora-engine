#include <doctest/doctest.h>

#include <yqlora/render/FrameBuilder.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Scene.hpp>

using namespace yqlora;

TEST_CASE("Camera3D: view matrix moves the world opposite to the camera")
{
    Camera3D camera;
    camera.transform().setPosition({0.0f, 0.0f, 5.0f});

    // Начало координат оказывается в 5 единицах перед камерой (по -Z).
    const Vec4 origin = camera.viewMatrix() * Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    CHECK(origin.z == doctest::Approx(-5.0f));
}

TEST_CASE("Camera3D: projection depth range depends on the API")
{
    Camera3D camera;
    camera.setClipPlanes(1.0f, 100.0f);

    auto ndcDepth = [&](DepthRange range, float viewZ) {
        const Vec4 clip = camera.projectionMatrix(1.0f, range) * Vec4(0.0f, 0.0f, viewZ, 1.0f);

        return clip.z / clip.w;
    };

    CHECK(ndcDepth(DepthRange::MinusOneToOne, -1.0f) == doctest::Approx(-1.0f));
    CHECK(ndcDepth(DepthRange::MinusOneToOne, -100.0f) == doctest::Approx(1.0f));
    CHECK(ndcDepth(DepthRange::ZeroToOne, -1.0f) == doctest::Approx(0.0f));
    CHECK(ndcDepth(DepthRange::ZeroToOne, -100.0f) == doctest::Approx(1.0f));
}

TEST_CASE("Scene: frame data comes from the active camera")
{
    Scene scene;
    auto& camera = scene.createRoot<Node>("Root").addChild<Camera3D>("MainCamera");
    camera.transform().setPosition({1.0f, 2.0f, 3.0f});
    scene.setActiveCamera(&camera);
    scene.setClearColor({0.1f, 0.2f, 0.3f, 1.0f});

    const FrameData frame = buildFrameData(scene, 16.0f / 9.0f, DepthRange::MinusOneToOne);
    CHECK(frame.camera.position.x == doctest::Approx(1.0f));
    CHECK(frame.camera.position.z == doctest::Approx(3.0f));
    CHECK(frame.clearColor.g == doctest::Approx(0.2f));
}
