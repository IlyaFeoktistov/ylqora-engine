#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace yqlora::editor {

class EditorSession;

// Где в проекте стоят узлы на одном слое.
struct LayerUsage {
    std::size_t nodes = 0;
    // Сцены, в которых такие узлы есть, пути от папки проекта.
    std::vector<std::filesystem::path> scenes;
};

// Узлы на слое layer во всех сценах проекта. Открытая сцена берётся из
// сессии, вместе с несохранёнными правками, остальные читаются из файлов.
// Маски камер не считаются: маска "Все" включает и слои без имени.
LayerUsage layerUsage(const EditorSession& session, int layer);

// Номера слоёв, у которых имя было, а в after стало пустым.
std::vector<int> clearedLayers(std::span<const std::string> before, std::span<const std::string> after);

} // namespace yqlora::editor
