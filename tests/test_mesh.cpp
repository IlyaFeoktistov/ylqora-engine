#include <doctest/doctest.h>

#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/render/Camera3D.hpp>
#include <yqlora/render/FrameBuilder.hpp>
#include <yqlora/render/MeshData.hpp>
#include <yqlora/render/MeshRenderer.hpp>
#include <yqlora/resource/BuiltinResources.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <glm/geometric.hpp>

using namespace yqlora;

TEST_CASE("MeshData: builtin meshes are valid and face outwards")
{
    for (const BuiltinMesh& builtinMesh : builtinMeshes()) {
        CAPTURE(builtinMesh.name);
        const MeshData* mesh = findBuiltinMesh(builtinMesh.id);
        REQUIRE(mesh);
        REQUIRE(mesh->indices.size() % 3 == 0);

        for (const std::uint32_t index : mesh->indices) {
            REQUIRE(index < mesh->vertices.size());
        }
        for (const Vertex& vertex : mesh->vertices) {
            CHECK(glm::length(vertex.normal) == doctest::Approx(1.0f));
        }

        // Против часовой стрелки снаружи: нормаль треугольника по порядку вершин
        // смотрит туда же, куда нормали вершин. Иначе грань отсечётся как задняя.
        for (std::size_t i = 0; i < mesh->indices.size(); i += 3) {
            const Vertex& a = mesh->vertices[mesh->indices[i]];
            const Vertex& b = mesh->vertices[mesh->indices[i + 1]];
            const Vertex& c = mesh->vertices[mesh->indices[i + 2]];
            const Vec3 faceNormal = glm::cross(b.position - a.position, c.position - a.position);
            // У полюсов сферы треугольники вырождаются в линию, их пропускаем.
            if (glm::length(faceNormal) < 1e-6f) {
                continue;
            }
            CHECK(glm::dot(faceNormal, a.normal + b.normal + c.normal) > 0.0f);
        }
    }

    CHECK(findBuiltinMesh(0) == nullptr);
    CHECK(findBuiltinMesh(builtin::kFirstProjectId) == nullptr);
}

TEST_CASE("FrameBuilder: every Node3D with a MeshRenderer and a known mesh becomes a draw item")
{
    Scene scene{EngineServices::none()};
    auto& root = scene.createRoot<Node3D>("Root");
    root.transform().setPosition({0.0f, 1.0f, 0.0f});

    auto& cube = root.addChild<Node3D>("Cube");
    cube.transform().setPosition({2.0f, 0.0f, 0.0f});
    auto& cubeRenderer = cube.addComponent<MeshRenderer>();
    cubeRenderer.setMesh(ResourceRef{builtin::kCubeMesh});
    cubeRenderer.setColor({1.0f, 0.0f, 0.0f, 1.0f});

    auto& unknown = root.addChild<Node3D>("Unknown");
    unknown.addComponent<MeshRenderer>().setMesh(ResourceRef{123456});

    const FrameData frame = extractFrame(scene, 1.0f, DepthRange::MinusOneToOne);
    REQUIRE(frame.drawItems.size() == 1);
    const DrawItem& item = frame.drawItems[0];
    CHECK(item.mesh == findBuiltinMesh(builtin::kCubeMesh));
    CHECK(item.node == cube.id());
    CHECK(item.color == Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    // Мировая матрица учитывает родителя.
    CHECK(Vec3(item.world[3]) == Vec3(2.0f, 1.0f, 0.0f));
}

TEST_CASE("MeshRenderer: mesh and color are saved with the scene")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);

    Scene original{EngineServices::none()};
    auto& ball = original.createRoot<Node3D>("Ball");
    auto& renderer = ball.addComponent<MeshRenderer>();
    renderer.setMesh(ResourceRef{builtin::kSphereMesh});
    renderer.setColor({0.2f, 0.4f, 0.6f, 1.0f});

    Scene loaded{EngineServices::none()};
    REQUIRE(serializer.fromJson(loaded, serializer.toJson(original)));
    const auto* loadedRenderer = loaded.root()->getComponent<MeshRenderer>();
    REQUIRE(loadedRenderer);
    CHECK(loadedRenderer->mesh().id == builtin::kSphereMesh);
    CHECK(loadedRenderer->color() == Vec4(0.2f, 0.4f, 0.6f, 1.0f));
}

TEST_CASE("SceneSerializer: a scene from a newer engine is not opened")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);

    Scene scene{EngineServices::none()};
    CHECK_FALSE(serializer.fromJson(scene, R"({"version": 99, "root": null})"));
    CHECK_FALSE(serializer.fromJson(scene, R"({"root": null})"));
}
