#pragma once

#include "yqlora/resource/ResourceId.hpp"
#include "yqlora/scene/NodeId.hpp"

#include <optional>
#include <string>

namespace yqlora::editor {

// Перетаскивание в редакторе. Источник и цель - последний элемент ImGui.

// Ресурс проекта (модель, меш, текстура) из "Файлов" - в дерево сцены, окно
// вида или поле ссылки на ресурс.

// Делает последний элемент ImGui источником: тянется ресурс id, рядом с
// курсором - label.
void resourceDragSource(ResourceId id, const std::string& label);

// Делает последний элемент ImGui целью: ID ресурса, если его отпустили здесь.
std::optional<ResourceId> acceptResourceDrop();

// Узел дерева сцены - к другому родителю.
void nodeDragSource(NodeId id, const std::string& label);
std::optional<NodeId> acceptNodeDrop();

} // namespace yqlora::editor
