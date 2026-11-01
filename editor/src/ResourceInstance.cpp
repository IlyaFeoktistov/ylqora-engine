#include "yqlora/editor/ResourceInstance.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/import/ModelData.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/render/MeshRenderer.hpp"
#include "yqlora/resource/ResourceCache.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"
#include "yqlora/resource/ResourceLoader.hpp"
#include "yqlora/scene/Node3D.hpp"

#include <map>
#include <string>

namespace yqlora::editor {

namespace {

// Узел без имени в файле.
constexpr const char* kUnnamedNode = "Node3D";
constexpr const char* kUnnamedMesh = "Mesh";

std::unique_ptr<Node3D> meshNode(std::string name, ResourceId mesh)
{
    auto node = std::make_unique<Node3D>();
    node->setName(std::move(name));
    node->addComponent<MeshRenderer>().setMesh(ResourceRef{mesh});

    return node;
}

std::string meshName(const ResourceDatabase& resources, ResourceId mesh)
{
    const std::string_view name = subresourceName(resources.key(mesh));

    return name.empty() ? kUnnamedMesh : std::string(name);
}

void addModelNode(
    Node3D& parent,
    const ModelData& model,
    std::size_t index,
    const std::map<std::string, ResourceId>& meshes,
    std::size_t depth
)
{
    // Испорченный файл мог бы зациклить дерево на себя.
    if (index >= model.nodes.size() || depth > model.nodes.size()) {
        return;
    }
    const ModelNode& source = model.nodes[index];
    auto node = std::make_unique<Node3D>();
    node->setName(source.name.empty() ? kUnnamedNode : source.name);
    node->transform().setPosition(source.position);
    node->transform().setRotation(source.rotation);
    node->transform().setScale(source.scale);
    if (source.mesh && *source.mesh < model.meshes.size()) {
        if (const auto mesh = meshes.find(model.meshes[*source.mesh]); mesh != meshes.end()) {
            node->addComponent<MeshRenderer>().setMesh(ResourceRef{mesh->second});
        }
    }
    Node3D& added = *node;
    parent.addChild(std::move(node));
    for (const std::size_t child : source.children) {
        addModelNode(added, model, child, meshes, depth + 1);
    }
}

} // namespace

std::unique_ptr<Node> instantiateResource(ResourceId id, ResourceCache& cache, const ResourceDatabase& resources)
{
    if (const ModelData* model = cache.find<ModelData>(id)) {
        auto root = std::make_unique<Node3D>();
        const auto path = resources.path(id);
        root->setName(path ? toUtf8(path->stem()) : kUnnamedNode);
        const auto& meshes = resources.subresources(id);
        for (const std::size_t index : model->roots) {
            addModelNode(*root, *model, index, meshes, 0);
        }

        return root;
    }
    if (cache.find<MeshData>(id)) {
        return meshNode(meshName(resources, id), id);
    }

    return nullptr;
}

} // namespace yqlora::editor
