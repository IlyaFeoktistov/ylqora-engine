#pragma once

#include <imgui.h>

namespace yqlora::editor {

struct EditorColors;

// Рамка вокруг кадра, пока идёт игра: в окне игры и в окне вида, где правки
// относятся к копии Play и пропадут после Stop.
void drawPlayFrame(const ImVec2& origin, const ImVec2& size, const EditorColors& colors);

} // namespace yqlora::editor
