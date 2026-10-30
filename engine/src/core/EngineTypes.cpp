#include "yqlora/core/EngineTypes.hpp"

#include "yqlora/project/ProjectTypes.hpp"
#include "yqlora/render/RenderTypes.hpp"
#include "yqlora/scene/SceneTypes.hpp"

namespace yqlora {

void registerEngineTypes(TypeRegistry& registry)
{
    registerSceneTypes(registry);
    registerRenderTypes(registry);
    registerProjectTypes(registry);
}

} // namespace yqlora
