#include "widgets/PlayFrame.hpp"

#include "style/Theme.hpp"

namespace yqlora::editor {

namespace {

constexpr float kPlayFrameThickness = 2.0f;

} // namespace

void drawPlayFrame(const ImVec2& origin, const ImVec2& size, const EditorColors& colors)
{
    ImGui::GetWindowDrawList()->AddRect(
        origin,
        ImVec2(origin.x + size.x, origin.y + size.y),
        toImU32(colors.playMode),
        0.0f,
        0,
        kPlayFrameThickness
    );
}

} // namespace yqlora::editor
