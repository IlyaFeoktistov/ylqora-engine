#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>

namespace yqlora {

class SceneSerializer;

// Переводит проект старого формата на текущий: все сцены и файл проекта.
// После этого старые версии движка его не откроют. Сцены сначала читаются
// все, и только если все прочитались, записываются: при ошибке проект
// остаётся как был. serializer должен знать типы игры, как при открытии.
YQLORA_API bool upgradeProject(const std::filesystem::path& directory, const SceneSerializer& serializer);

} // namespace yqlora
