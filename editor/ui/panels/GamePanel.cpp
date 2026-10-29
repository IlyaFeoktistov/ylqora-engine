#include "panels/GamePanel.hpp"

#include "style/Theme.hpp"
#include "widgets/PlayFrame.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/scene/RenderSystem.hpp"
#include "yqlora/scene/Scene.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <string>

namespace yqlora::editor {

void GamePanel::draw(EditorContext& context)
{
    EditorSession& session = context.session;

    // В раскладке, сохранённой до появления окна игры, его нет: первый раз
    // встаёт вкладкой рядом с окном вида, а не висит посреди экрана.
    if (const ImGuiWindow* viewport = ImGui::FindWindowByName("###Viewport")) {
        ImGui::SetNextWindowDockID(viewport->DockId, ImGuiCond_FirstUseEver);
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin(title().c_str());
    ImGui::PopStyleVar();

    // Игра началась: окно игры выходит вперёд и сразу получает ввод.
    const bool playing = session.playing();
    if (playing && !m_wasPlaying) {
        ImGui::SetWindowFocus();
    }
    m_wasPlaying = playing;

    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (!visible || !m_view.resize(context.app, size)) {
        ImGui::End();

        return;
    }
    session.setGameInputFocused(playing && ImGui::IsWindowFocused());

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    m_view.image(size);

    const Scene& scene = session.scene();
    const RenderSystem* render = scene.system<RenderSystem>();
    if (render && render->mainCamera()) {
        m_view.render(context.app, extractFrame(scene, size.x / size.y, context.app.depthRange()));
    } else {
        // Без камеры игре не через что показываться.
        const std::string text = tr("editor.play.noCamera");
        const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImGui::GetWindowDrawList()->AddRectFilled(
            origin,
            ImVec2(origin.x + size.x, origin.y + size.y),
            toImU32(context.theme.colors().viewportBackground)
        );
        ImGui::GetWindowDrawList()->AddText(
            ImVec2(origin.x + (size.x - textSize.x) * 0.5f, origin.y + (size.y - textSize.y) * 0.5f),
            ImGui::GetColorU32(ImGuiCol_TextDisabled),
            text.c_str()
        );
    }
    if (playing) {
        drawPlayFrame(origin, size, context.theme.colors());
    }
    ImGui::End();
}

} // namespace yqlora::editor
