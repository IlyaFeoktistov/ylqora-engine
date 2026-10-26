#include <doctest/doctest.h>

#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/render/FrameBuilder.hpp>
#include <yqlora/render/MeshData.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/MeshInstance3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <glm/geometric.hpp>

using namespace yqlora;

TEST_CASE("MeshData: builtin meshes are valid and face outwards")
{
    for (const char* name : {"cube", "sphere", "plane"}) {
        CAPTURE(name);
        const MeshData* mesh = findBuiltinMesh(name);
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

    CHECK(findBuiltinMesh("teapot") == nullptr);
}

TEST_CASE("FrameBuilder: every MeshInstance3D with a known mesh becomes a draw item")
{
    Scene scene;
    auto& root = scene.createRoot<Node3D>("Root");
    root.transform().setPosition({0.0f, 1.0f, 0.0f});

    auto& cube = root.addChild<MeshInstance3D>("Cube");
    cube.transform().setPosition({2.0f, 0.0f, 0.0f});
    cube.setColor({1.0f, 0.0f, 0.0f, 1.0f});

    auto& unknown = root.addChild<MeshInstance3D>("Unknown");
    unknown.setMesh("teapot");

    const FrameData frame = buildFrameData(scene, 1.0f, DepthRange::MinusOneToOne);
    REQUIRE(frame.drawItems.size() == 1);
    const DrawItem& item = frame.drawItems[0];
    CHECK(item.mesh == findBuiltinMesh("cube"));
    CHECK(item.node == cube.id());
    CHECK(item.color == Vec4(1.0f, 0.0f, 0.0f, 1.0f));
    // Мировая матрица учитывает родителя.
    CHECK(Vec3(item.world[3]) == Vec3(2.0f, 1.0f, 0.0f));
}

TEST_CASE("MeshInstance3D: mesh and color are saved with the scene")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    const SceneSerializer serializer(registry);

    Scene original;
    auto& sphere = original.createRoot<MeshInstance3D>("Ball");
    sphere.setMesh("sphere");
    sphere.setColor({0.2f, 0.4f, 0.6f, 1.0f});

    Scene loaded;
    REQUIRE(serializer.fromJson(loaded, serializer.toJson(original)));
    auto* loadedSphere = dynamic_cast<MeshInstance3D*>(loaded.root());
    REQUIRE(loadedSphere);
    CHECK(loadedSphere->mesh() == "sphere");
    CHECK(loadedSphere->color() == Vec4(0.2f, 0.4f, 0.6f, 1.0f));
}
