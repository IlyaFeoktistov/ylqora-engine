#include "yqlora/editor/ResourceChoices.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/editor/ProjectFiles.hpp"
#include "yqlora/editor/ResourceLabel.hpp"
#include "yqlora/import/ModelData.hpp"
#include "yqlora/render/BuiltinMeshes.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/render/TextureData.hpp"
#include "yqlora/resource/ResourceCache.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"

namespace yqlora::editor {

namespace {

// Вид файла верхнего уровня: по загрузчику, а сцену загрузчик не читает.
std::string_view fileKind(const std::filesystem::path& path, ResourceCache& cache)
{
    return ProjectFiles::isScene(path) ? kSceneKind : cache.kindOf(path);
}

// Меши модели file в порядке файла; те, которых в файле уже нет, не идут.
std::vector<std::pair<std::string, ResourceId>> modelMeshes(
    ResourceId file,
    const ResourceDatabase& resources,
    ResourceCache& cache
)
{
    std::vector<std::pair<std::string, ResourceId>> meshes;
    const ModelData* model = cache.find<ModelData>(file);
    if (!model) {
        return meshes;
    }
    const auto& ids = resources.subresources(file);
    for (const std::string& key : model->meshes) {
        if (const auto id = ids.find(key); id != ids.end() && cache.find<MeshData>(id->second)) {
            meshes.emplace_back(key, id->second);
        }
    }

    return meshes;
}

} // namespace

std::string_view fieldKind(PropertyHint hint)
{
    switch (hint) {
    case PropertyHint::Mesh: return MeshData::kResourceKind;
    case PropertyHint::Texture: return TextureData::kResourceKind;
    case PropertyHint::Scene: return kSceneKind;
    default: return {};
    }
}

std::vector<ResourceChoice> resourceChoices(PropertyHint hint, const ResourceDatabase* resources, ResourceCache& cache)
{
    const std::string_view kind = fieldKind(hint);
    std::vector<ResourceChoice> choices;
    if (kind == MeshData::kResourceKind) {
        for (const BuiltinMesh& mesh : builtinMeshes()) {
            choices.push_back({mesh.id, std::string(mesh.name), {}, true});
        }
    }
    if (!resources || kind.empty()) {
        return choices;
    }
    for (const auto& [file, path] : resources->paths()) {
        const std::string_view type = fileKind(path, cache);
        if (kind == MeshData::kResourceKind && type == ModelData::kResourceKind) {
            for (const auto& [key, id] : modelMeshes(file, *resources, cache)) {
                choices.push_back({id, subresourceLabel(key), toGenericUtf8(path), false});
            }
        } else if (type == kind) {
            choices.push_back({file, toGenericUtf8(path), {}, false});
        }
    }

    return choices;
}

std::optional<ResourceId> resourceForField(
    PropertyHint hint,
    ResourceId id,
    const ResourceDatabase* resources,
    ResourceCache& cache
)
{
    const std::string_view kind = fieldKind(hint);
    if (kind == MeshData::kResourceKind && cache.find<MeshData>(id)) {
        return id;
    }
    const auto path = resources ? resources->path(id) : std::nullopt;
    if (!path || !resources->key(id).empty()) {
        return std::nullopt;
    }
    const std::string_view type = fileKind(*path, cache);
    if (kind == MeshData::kResourceKind && type == ModelData::kResourceKind) {
        const auto meshes = modelMeshes(id, *resources, cache);

        return meshes.empty() ? std::nullopt : std::optional(meshes.front().second);
    }

    return type == kind ? std::optional(id) : std::nullopt;
}

} // namespace yqlora::editor
