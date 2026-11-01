#pragma once

#include "yqlora/resource/ResourceId.hpp"

#include <optional>
#include <string>

namespace yqlora::editor {

// Перетаскивание ресурса проекта (модели, меша) из "Файлов" в дерево сцены,
// окно вида или поле ссылки на ресурс.

// Делает последний элемент ImGui источником: тянется ресурс id, рядом с
// курсором - label.
void resourceDragSource(ResourceId id, const std::string& label);

// Делает последний элемент ImGui целью: ID ресурса, если его отпустили здесь.
std::optional<ResourceId> acceptResourceDrop();

} // namespace yqlora::editor
