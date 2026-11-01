#include "yqlora/import/EngineLoaders.hpp"

#include "yqlora/import/GltfLoader.hpp"
#include "yqlora/resource/ResourceCache.hpp"

#include <memory>

namespace yqlora {

void addEngineLoaders(ResourceCache& cache)
{
    cache.addLoader(std::make_unique<GltfLoader>());
}

} // namespace yqlora
