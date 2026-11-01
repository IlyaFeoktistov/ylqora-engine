#pragma once

#include "yqlora/reflection/Property.hpp"

#include <string>
#include <string_view>

namespace yqlora {
class ResourceDatabase;
} // namespace yqlora

namespace yqlora::editor {

// Подпись вложенного ресурса по его ключу, без вида: "Door", "Door#1", "#2".
std::string subresourceLabel(std::string_view key);

// Как ссылка на ресурс выглядит в инспекторе: имя встроенного меша, путь
// файла в проекте ("models/door.glb / Door" у вложенного), "нет" для пустой
// ссылки или "не найден".
// resources - ресурсы открытого проекта, nullptr без проекта.
std::string resourceLabel(ResourceRef value, const ResourceDatabase* resources);

} // namespace yqlora::editor
