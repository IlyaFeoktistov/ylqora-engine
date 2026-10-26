#pragma once

#include "yqlora/math/Math.hpp"

#include <filesystem>

namespace yqlora {

// Цвета редактора, которых нет среди цветов ImGui.
struct EditorColors {
    Vec4 errorText{1.0f, 0.45f, 0.4f, 1.0f};
    Vec4 selectionOutline{1.0f, 0.6f, 0.1f, 1.0f};
    // Контур камер в окне сцены.
    Vec4 cameraGizmo{0.78f, 0.8f, 0.84f, 1.0f};
    // Подписи X, Y, Z, W у полей векторов.
    Vec4 axisX{0.9f, 0.35f, 0.33f, 1.0f};
    Vec4 axisY{0.45f, 0.75f, 0.35f, 1.0f};
    Vec4 axisZ{0.35f, 0.55f, 0.95f, 1.0f};
    Vec4 axisW{0.6f, 0.6f, 0.6f, 1.0f};
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
