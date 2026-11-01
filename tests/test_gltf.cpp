#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/core/Utf8Path.hpp>
#include <yqlora/import/GltfLoader.hpp>
#include <yqlora/import/ModelData.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/project/Project.hpp>
#include <yqlora/render/MeshData.hpp>
#include <yqlora/resource/ResourceCache.hpp>

#include <glm/geometric.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace yqlora;
namespace fs = std::filesystem;
using Json = nlohmann::json;

namespace {

// Коды glTF.
constexpr int kFloat = 5126;
constexpr int kUnsignedShort = 5123;
constexpr int kTriangles = 4;
constexpr int kPoints = 0;
constexpr std::uint32_t kGlbMagic = 0x46546C67;
constexpr std::uint32_t kGlbVersion = 2;
constexpr std::uint32_t kJsonChunk = 0x4E4F534A;
constexpr std::uint32_t kBinChunk = 0x004E4942;

std::string base64(const std::vector<std::uint8_t>& bytes)
{
    static constexpr char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string text;
    for (std::size_t i = 0; i < bytes.size(); i += 3) {
        const std::uint32_t chunk = (bytes[i] << 16) | (i + 1 < bytes.size() ? bytes[i + 1] << 8 : 0)
                                    | (i + 2 < bytes.size() ? bytes[i + 2] : 0);
        text += kAlphabet[(chunk >> 18) & 63];
        text += kAlphabet[(chunk >> 12) & 63];
        text += i + 1 < bytes.size() ? kAlphabet[(chunk >> 6) & 63] : '=';
        text += i + 2 < bytes.size() ? kAlphabet[chunk & 63] : '=';
    }

    return text;
}

// Собирает glTF: двоичный буфер, виды и аксессоры, остальное - JSON руками.
struct GltfBuilder {
    template <typename T>
    int view(const std::vector<T>& values, int stride = 0)
    {
        while (bin.size() % 4 != 0) {
            bin.push_back(0);
        }
        const std::size_t offset = bin.size();
        bin.resize(offset + values.size() * sizeof(T));
        std::memcpy(bin.data() + offset, values.data(), values.size() * sizeof(T));
        Json entry{{"buffer", 0}, {"byteOffset", offset}, {"byteLength", values.size() * sizeof(T)}};
        if (stride != 0) {
            entry["byteStride"] = stride;
        }
        json["bufferViews"].push_back(entry);

        return static_cast<int>(json["bufferViews"].size()) - 1;
    }

    int accessor(int bufferView, int offset, int componentType, int count, const char* type, Json extra = {})
    {
        Json entry{
            {"bufferView", bufferView},
            {"byteOffset", offset},
            {"componentType", componentType},
            {"count", count},
            {"type", type}
        };
        if (extra.is_object()) {
            entry.update(extra);
        }
        json["accessors"].push_back(entry);

        return static_cast<int>(json["accessors"].size()) - 1;
    }

    // Аксессор VEC3 из чисел подряд, с границами: их требует glTF для положений.
    int positions(const std::vector<float>& values)
    {
        Vec3 low(values[0], values[1], values[2]);
        Vec3 high = low;
        for (std::size_t i = 0; i < values.size(); i += 3) {
            low = glm::min(low, Vec3(values[i], values[i + 1], values[i + 2]));
            high = glm::max(high, Vec3(values[i], values[i + 1], values[i + 2]));
        }
        const int count = static_cast<int>(values.size() / 3);

        return accessor(
            view(values),
            0,
            kFloat,
            count,
            "VEC3",
            {{"min", {low.x, low.y, low.z}}, {"max", {high.x, high.y, high.z}}}
        );
    }

    Json document(Json buffer) const
    {
        Json result = json;
        result["asset"] = {{"version", "2.0"}};
        buffer["byteLength"] = bin.size();
        result["buffers"] = Json::array({buffer});

        return result;
    }

    void writeGltf(const fs::path& file) const
    {
        std::ofstream(file) << document({{"uri", "data:application/octet-stream;base64," + base64(bin)}}).dump();
    }

    // .gltf и рядом внешний .bin.
    void writeGltfWithBin(const fs::path& file, const std::string& binName) const
    {
        std::ofstream(file.parent_path() / fromUtf8(binName), std::ios::binary)
            .write(reinterpret_cast<const char*>(bin.data()), static_cast<std::streamsize>(bin.size()));
        std::ofstream(file) << document({{"uri", binName}}).dump();
    }

    void writeGlb(const fs::path& file) const
    {
        std::string text = document(Json::object()).dump();
        while (text.size() % 4 != 0) {
            text += ' ';
        }
        std::vector<std::uint8_t> data = bin;
        while (data.size() % 4 != 0) {
            data.push_back(0);
        }
        const auto word = [](std::ofstream& out, std::uint32_t value) {
            out.write(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        std::ofstream out(file, std::ios::binary);
        word(out, kGlbMagic);
        word(out, kGlbVersion);
        word(out, static_cast<std::uint32_t>(12 + 8 + text.size() + 8 + data.size()));
        word(out, static_cast<std::uint32_t>(text.size()));
        word(out, kJsonChunk);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        word(out, static_cast<std::uint32_t>(data.size()));
        word(out, kBinChunk);
        out.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    }

    Json json = Json::object();
    std::vector<std::uint8_t> bin;
};

fs::path freshDirectory(const char* name)
{
    const auto directory = fs::temp_directory_path() / "yqlora_tests" / name;
    fs::remove_all(directory);
    fs::create_directories(directory);

    return directory;
}

LoadResult load(const fs::path& file, Json import = Json::object())
{
    return GltfLoader{}.load({file, std::move(import)});
}

template <typename T>
const T& data(const LoadedResource& resource)
{
    REQUIRE(resource.kind == T::kResourceKind);

    return *static_cast<const T*>(resource.data.get());
}

// Один треугольник в плоскости XY без нормалей и без индексов.
GltfBuilder plainTriangle()
{
    GltfBuilder gltf;
    const int positions = gltf.positions({0, 0, 0, 1, 0, 0, 0, 1, 0});
    gltf.json["meshes"] = {{{"name", "Tri"}, {"primitives", {{{"attributes", {{"POSITION", positions}}}}}}}};
    gltf.json["nodes"] = {{{"name", "Root"}, {"mesh", 0}, {"translation", {1.0, 2.0, 3.0}}}};

    return gltf;
}

} // namespace

TEST_CASE("GltfLoader: embedded buffer, no indices, no normals: normals are computed, indices go in order")
{
    const fs::path file = freshDirectory("gltf_plain") / "tri.gltf";
    plainTriangle().writeGltf(file);

    const LoadResult result = load(file);
    REQUIRE(result.error.empty());
    REQUIRE(result.resources.size() == 2);
    CHECK(result.resources[0].key.empty());
    const auto& model = data<ModelData>(result.resources[0]);
    REQUIRE(model.nodes.size() == 1);
    CHECK(model.roots == std::vector<std::size_t>{0});
    CHECK(model.nodes[0].name == "Root");
    CHECK(model.nodes[0].position == Vec3(1.0f, 2.0f, 3.0f));
    CHECK(model.nodes[0].mesh == 0);
    CHECK(model.meshes == std::vector<std::string>{"mesh:Tri"});

    CHECK(result.resources[1].key == "mesh:Tri");
    const auto& mesh = data<MeshData>(result.resources[1]);
    CHECK(mesh.indices == std::vector<std::uint32_t>{0, 1, 2});
    for (const Vertex& vertex : mesh.vertices) {
        CHECK(vertex.normal == Vec3(0.0f, 0.0f, 1.0f));
        CHECK(vertex.uv0 == Vec2(0.0f));
    }
    REQUIRE(mesh.submeshes.size() == 1);
    CHECK_FALSE(mesh.submeshes[0].sourceMaterial);
    CHECK(mesh.bounds.max == Vec3(1.0f, 1.0f, 0.0f));
}

TEST_CASE("GltfLoader: GLB with interleaved data, shared and same-named meshes, materials and a skipped primitive")
{
    GltfBuilder gltf;
    // Положение и нормаль вперемежку, шаг 24 байта.
    const std::vector<float> interleaved{
        0, 0, 0, 0, 0, 1, //
        2, 0, 0, 0, 0, 1, //
        0, 2, 0, 0, 0, 1, //
        2, 2, 0, 0, 0, 1,
    };
    const int vertexView = gltf.view(interleaved, 24);
    const Json bounds{{"min", {0.0, 0.0, 0.0}}, {"max", {2.0, 2.0, 0.0}}};
    const int positions = gltf.accessor(vertexView, 0, kFloat, 4, "VEC3", bounds);
    const int normals = gltf.accessor(vertexView, 12, kFloat, 4, "VEC3");
    const int indices = gltf.accessor(
        gltf.view(std::vector<std::uint16_t>{0, 1, 2, 1, 3, 2}),
        0,
        kUnsignedShort,
        6,
        "SCALAR"
    );
    const Json attributes{{"POSITION", positions}, {"NORMAL", normals}};
    gltf.json["materials"] = {{{"name", "Wood"}}, {{"name", "Metal"}}};
    gltf.json["meshes"] = {
        {{"name", "Door"},
         {"primitives",
          {{{"attributes", attributes}, {"indices", indices}, {"material", 1}, {"mode", kTriangles}},
           {{"attributes", attributes}, {"mode", kPoints}},
           {{"attributes", attributes}, {"indices", indices}}}}},
        {{"name", "Door"}, {"primitives", {{{"attributes", attributes}, {"indices", indices}}}}},
        {{"primitives", {{{"attributes", attributes}, {"indices", indices}, {"material", 0}}}}},
    };
    // Две двери с одним мешем; у рамы матрица вместо TRS.
    gltf.json["nodes"] = {
        {{"name", "House"}, {"children", {1, 2, 3}}},
        {{"name", "LeftDoor"}, {"mesh", 0}, {"translation", {-1.0, 0.0, 0.0}}},
        {{"name", "RightDoor"}, {"mesh", 0}, {"translation", {1.0, 0.0, 0.0}}, {"rotation", {0.0, 1.0, 0.0, 0.0}}},
        {{"name", "Frame"},
         {"mesh", 2},
         {"matrix", {2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 0.0, 0.0, 0.0, 2.0, 0.0, 5.0, 6.0, 7.0, 1.0}}},
    };
    gltf.json["scenes"] = {{{"nodes", {0}}}};
    gltf.json["scene"] = 0;
    const fs::path file = freshDirectory("gltf_glb") / "house.glb";
    gltf.writeGlb(file);

    const LoadResult result = load(file);
    REQUIRE(result.error.empty());
    REQUIRE(result.resources.size() == 4);
    const auto& model = data<ModelData>(result.resources[0]);
    CHECK(model.meshes == std::vector<std::string>{"mesh:Door", "mesh:Door#1", "mesh:#2"});
    CHECK(model.roots == std::vector<std::size_t>{0});
    CHECK(model.nodes[0].children == std::vector<std::size_t>{1, 2, 3});
    CHECK(model.nodes[1].mesh == model.nodes[2].mesh);
    CHECK(model.nodes[2].rotation.y == doctest::Approx(1.0f));
    CHECK(model.nodes[2].rotation.w == doctest::Approx(0.0f));
    CHECK(model.nodes[3].position == Vec3(5.0f, 6.0f, 7.0f));
    CHECK(model.nodes[3].scale.x == doctest::Approx(2.0f));
    CHECK_FALSE(model.nodes[0].mesh);

    // Точки пропущены, два треугольных примитива - два подмеша.
    const auto& door = data<MeshData>(result.resources[1]);
    CHECK(door.vertices.size() == 8);
    CHECK(door.vertices[3].position == Vec3(2.0f, 2.0f, 0.0f));
    CHECK(door.vertices[3].normal == Vec3(0.0f, 0.0f, 1.0f));
    REQUIRE(door.submeshes.size() == 2);
    CHECK(door.submeshes[0].sourceMaterial == 1);
    CHECK_FALSE(door.submeshes[1].sourceMaterial);
    CHECK(door.submeshes[1].firstIndex == 6);
    CHECK(door.indices[6] == 4);
    CHECK(data<MeshData>(result.resources[3]).submeshes[0].sourceMaterial == 0);
}

TEST_CASE("GltfLoader: an external buffer next to the file, in a folder with a non-ASCII name")
{
    const fs::path directory = freshDirectory("gltf_external") / fromUtf8("модели");
    fs::create_directories(directory);
    plainTriangle().writeGltfWithBin(directory / "tri.gltf", "tri.bin");

    const LoadResult result = load(directory / "tri.gltf");
    REQUIRE(result.error.empty());
    CHECK(data<MeshData>(result.resources[1]).vertices[1].position == Vec3(1.0f, 0.0f, 0.0f));
}

TEST_CASE("GltfLoader: sparse positions and normalized integer texture coordinates")
{
    GltfBuilder gltf;
    const int base = gltf.view(std::vector<float>{0, 0, 0, 1, 0, 0, 0, 1, 0});
    // Вершина 2 подменяется разреженным аксессором.
    const int sparseIndices = gltf.view(std::vector<std::uint16_t>{2, 0});
    const int sparseValues = gltf.view(std::vector<float>{0, 4, 0});
    const Json sparse{
        {"count", 1},
        {"indices", {{"bufferView", sparseIndices}, {"componentType", kUnsignedShort}}},
        {"values", {{"bufferView", sparseValues}}}
    };
    const int positions = gltf.accessor(
        base,
        0,
        kFloat,
        3,
        "VEC3",
        {{"sparse", sparse}, {"min", {0.0, 0.0, 0.0}}, {"max", {1.0, 4.0, 0.0}}}
    );
    const int uvs = gltf.accessor(
        gltf.view(std::vector<std::uint16_t>{0, 0, 65535, 0, 0, 65535}),
        0,
        kUnsignedShort,
        3,
        "VEC2",
        {{"normalized", true}}
    );
    gltf.json["meshes"] = {{{"primitives", {{{"attributes", {{"POSITION", positions}, {"TEXCOORD_0", uvs}}}}}}}};
    const fs::path file = freshDirectory("gltf_sparse") / "sparse.gltf";
    gltf.writeGltf(file);

    const LoadResult result = load(file);
    REQUIRE(result.error.empty());
    const auto& mesh = data<MeshData>(result.resources[1]);
    CHECK(mesh.vertices[2].position == Vec3(0.0f, 4.0f, 0.0f));
    CHECK(mesh.vertices[1].uv0 == Vec2(1.0f, 0.0f));
    CHECK(mesh.vertices[2].uv0 == Vec2(0.0f, 1.0f));
    // Без узлов и сцен модель пустая, но меш есть.
    CHECK(data<ModelData>(result.resources[0]).nodes.empty());
}

TEST_CASE("GltfLoader: the scale import setting scales vertices and node positions")
{
    const fs::path file = freshDirectory("gltf_scale") / "tri.gltf";
    plainTriangle().writeGltf(file);

    const LoadResult result = load(file, {{"scale", 0.5}});
    REQUIRE(result.error.empty());
    CHECK(data<ModelData>(result.resources[0]).nodes[0].position == Vec3(0.5f, 1.0f, 1.5f));
    const auto& mesh = data<MeshData>(result.resources[1]);
    CHECK(mesh.vertices[1].position == Vec3(0.5f, 0.0f, 0.0f));
    CHECK(mesh.bounds.max == Vec3(0.5f, 0.5f, 0.0f));
}

TEST_CASE("GltfLoader: broken files give an error, not a crash")
{
    const fs::path directory = freshDirectory("gltf_broken");
    std::ofstream(directory / "text.gltf") << "not json";
    CHECK_FALSE(load(directory / "text.gltf").error.empty());

    std::ofstream(directory / "empty.glb");
    CHECK_FALSE(load(directory / "empty.glb").error.empty());
    CHECK_FALSE(load(directory / "missing.gltf").error.empty());

    // Обрезанный GLB.
    plainTriangle().writeGlb(directory / "whole.glb");
    std::ifstream whole(directory / "whole.glb", std::ios::binary);
    std::string bytes{std::istreambuf_iterator<char>(whole), std::istreambuf_iterator<char>()};
    std::ofstream(directory / "cut.glb", std::ios::binary) << bytes.substr(0, bytes.size() / 2);
    CHECK_FALSE(load(directory / "cut.glb").error.empty());

    // Индекс за пределами вершин.
    GltfBuilder gltf;
    const int positions = gltf.positions({0, 0, 0, 1, 0, 0, 0, 1, 0});
    const int indices = gltf.accessor(gltf.view(std::vector<std::uint16_t>{0, 1, 7}), 0, kUnsignedShort, 3, "SCALAR");
    gltf.json["meshes"] = {{{"primitives", {{{"attributes", {{"POSITION", positions}}}, {"indices", indices}}}}}};
    gltf.writeGltf(directory / "index.gltf");
    CHECK_FALSE(load(directory / "index.gltf").error.empty());
}

TEST_CASE("ResourceCache: meshes of a glTF file are nested resources with ids in .meta")
{
    const fs::path directory = freshDirectory("gltf_cache");
    plainTriangle().writeGlb(directory / "tri.glb");
    test::EngineRegistry registry;
    REQUIRE(Project::create(directory, "Game", registry));
    auto project = Project::open(directory, registry);
    REQUIRE(project);
    Input input;
    EngineServices services{input, registry};
    services.setProject(&*project);

    const ResourceId file = project->resources().id("tri.glb");
    const ModelData* model = services.resources().find<ModelData>(file);
    REQUIRE(model);
    const ResourceId mesh = project->resources().subresources(file).at(model->meshes[0]);
    REQUIRE(services.resources().find<MeshData>(mesh));
    CHECK(services.resources().find<MeshData>(mesh)->vertices.size() == 3);
    // Модель - не меш.
    CHECK(services.resources().find<MeshData>(file) == nullptr);
    services.setProject(nullptr);
}
