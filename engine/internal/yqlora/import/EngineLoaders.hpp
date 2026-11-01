#pragma once

namespace yqlora {

class ResourceCache;

// Загрузчики файлов, которые знает движок (glTF, позже картинки).
void addEngineLoaders(ResourceCache& cache);

} // namespace yqlora
