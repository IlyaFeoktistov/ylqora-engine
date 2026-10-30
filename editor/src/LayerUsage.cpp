#include "yqlora/editor/LayerUsage.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/ProjectFiles.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <algorithm>

namespace yqlora::editor {

namespace {

std::size_t countOnLayer(const Node& node, int layer)
{
    std::size_t count = node.layer() == layer ? 1 : 0;
    for (const auto& child : node.children()) {
        count += countOnLayer(*child, layer);
    }

    return count;
}

} // namespace

LayerUsage layerUsage(const EditorSession& session, int layer)
{
    LayerUsage usage;
    if (!session.project()) {
        return usage;
    }
    const Project& project = *session.project();
    const ResourceId open = session.document().id();
    for (const auto& [id, path] : project.resources().paths()) {
        if (!ProjectFiles::isScene(path)) {
            continue;
        }
        std::size_t count = 0;
        if (id == open) {
            const Scene& scene = session.document().scene();
            count = scene.root() ? countOnLayer(*scene.root(), layer) : 0;
        } else {
            Scene scene{EngineServices::none()};
            if (session.serializer().load(scene, project.directory() / path) && scene.root()) {
                count = countOnLayer(*scene.root(), layer);
            }
        }
        if (count > 0) {
            usage.nodes += count;
            usage.scenes.push_back(path);
        }
    }

    return usage;
}

std::vector<int> clearedLayers(std::span<const std::string> before, std::span<const std::string> after)
{
    std::vector<int> result;
    const std::size_t count = std::min(before.size(), after.size());
    for (std::size_t i = 0; i < count; ++i) {
        if (!before[i].empty() && after[i].empty()) {
            result.push_back(static_cast<int>(i));
        }
    }

    return result;
}

} // namespace yqlora::editor
