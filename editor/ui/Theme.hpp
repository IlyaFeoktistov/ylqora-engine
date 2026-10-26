#pragma once

#include "yqlora/math/Math.hpp"

#include <filesystem>

namespace yqlora {

// Цвета редактора, которых нет среди цветов ImGui.
struct EditorColors {
    Vec4 errorText{1.0f, 0.45f, 0.4f, 1.0f};
    Vec4 selectionOutline{1.0f, 0.6f, 0.1f, 1.0f};
};

// Внешний вид интерфейса задаётся файлом темы (assets/themes/*.json), а не кодом:
// шрифт, размеры и отступы, цвета ImGui и цвета редактора. Чего нет в файле,
// остаётся как в стандартной тёмной теме ImGui.
namespace theme {

// Применяет тему к ImGui. Вызывать до первого кадра интерфейса: шрифт
// добавляется в атлас ImGui. assetsDir - папка, от которой считается путь к шрифту.
bool load(const std::filesystem::path& file, const std::filesystem::path& assetsDir);

const EditorColors& colors();

} // namespace theme

} // namespace yqlora
