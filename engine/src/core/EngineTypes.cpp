#include "yqlora/core/EngineTypes.hpp"

#include "yqlora/project/ProjectTypes.hpp"
#include "yqlora/render/RenderTypes.hpp"
#include "yqlora/scene/SceneTypes.hpp"

namespace yqlora {

void registerEngineTypes(TypeRegistry& registry)
{
    // Первыми: корень разделов настроек нужен разделам подсистем.
    registerProjectTypes(registry);
    registerSceneTypes(registry);
    registerRenderTypes(registry);
}

} // namespace yqlora
