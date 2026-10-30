#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>

namespace yqlora {

class SceneSerializer;

// Переводит проект старого формата на текущий: все сцены, файлы разделов
// настроек и файл проекта. После этого старые версии движка его не откроют.
// Сцены и разделы сначала читаются все, и только если все прочитались,
// записываются: при ошибке чтения проект остаётся как был. Файл проекта
// пишется последним, поэтому прерванный перевод можно просто повторить.
// serializer должен знать типы игры, как при открытии.
YQLORA_API bool upgradeProject(const std::filesystem::path& directory, const SceneSerializer& serializer);

} // namespace yqlora
