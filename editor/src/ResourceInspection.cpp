#include "yqlora/editor/ResourceInspection.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/ResourceLabel.hpp"
#include "yqlora/import/GltfLoader.hpp"
#include "yqlora/import/ModelData.hpp"
#include "yqlora/import/TextureImport.hpp"
#include "yqlora/render/MeshData.hpp"

#include <nlohmann/json.hpp>

namespace yqlora::editor {

namespace {

constexpr std::size_t kTriangleCorners = 3;

nlohmann::json importJson(const ResourceDatabase& resources, ResourceId id)
{
    nlohmann::json json = nlohmann::json::parse(resources.importSettings(id), nullptr, false);

    return json.is_object() ? json : nlohmann::json::object();
}

bool apply(EditorSession& session, ResourceId id, const nlohmann::json& settings)
{
    auto& project = session.project();
    if (!project || !project->resources().setImportSettings(id, settings.dump())) {
        return false;
    }

    return session.resources().reload(id);
}

} // namespace

std::optional<ResourceInspection> inspectResource(EditorSession& session, ResourceId id)
{
    const auto& project = session.project();
    const auto path = project ? project->resources().path(id) : std::nullopt;
    if (!path || !project->resources().key(id).empty()) {
        return std::nullopt;
    }
    ResourceCache& cache = session.resources();
    ResourceInspection inspection;
    inspection.id = id;
    inspection.path = *path;
    inspection.kind = cache.kindOf(*path);
    if (inspection.kind.empty()) {
        return inspection;
    }
    const nlohmann::json import = importJson(project->resources(), id);

    if (inspection.kind == ModelData::kResourceKind) {
        inspection.modelImport = ModelImport{import.value(GltfLoader::kScaleSetting, GltfLoader::kDefaultScale)};
        if (const ModelData* model = cache.find<ModelData>(id)) {
            inspection.nodes = model->nodes.size();
            const auto& ids = project->resources().subresources(id);
            for (const std::string& key : model->meshes) {
                const auto mesh = ids.find(key);
                const MeshData* data = mesh != ids.end() ? cache.find<MeshData>(mesh->second) : nullptr;
                if (data) {
                    inspection.meshes.push_back({
                        .id = mesh->second,
                        .label = subresourceLabel(key),
                        .vertices = data->vertices.size(),
                        .triangles = data->indices.size() / kTriangleCorners,
                        .submeshes = data->submeshes.size(),
                    });
                }
            }
        }
    } else if (inspection.kind == TextureData::kResourceKind) {
        inspection.textureImport = textureSamplingFromJson(import);
        inspection.texture = cache.get<TextureData>(id);
    }
    if (const ResourceStatus* status = cache.status(id)) {
        inspection.status = *status;
    }

    return inspection;
}

bool applyImportSettings(EditorSession& session, ResourceId id, const ModelImport& settings)
{
    auto& project = session.project();
    if (!project) {
        return false;
    }
    nlohmann::json json = importJson(project->resources(), id);
    if (settings.scale == GltfLoader::kDefaultScale) {
        json.erase(GltfLoader::kScaleSetting);
    } else {
        json[GltfLoader::kScaleSetting] = settings.scale;
    }

    return apply(session, id, json);
}

bool applyImportSettings(EditorSession& session, ResourceId id, const TextureSampling& settings)
{
    return apply(session, id, toJson(settings));
}

} // namespace yqlora::editor
