#include "menu/MainMenu.hpp"

#include "core/ActionIds.hpp"
#include "core/Actions.hpp"
#include "core/EditorPanel.hpp"
#include "style/Icons.hpp"
#include "style/Theme.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Localization.hpp"
#include "yqlora/i18n/Translate.hpp"

#include <imgui.h>

namespace yqlora::editor {

namespace {

void drawPlayControls(const MainMenuContext& context)
{
    const ImGuiStyle& style = ImGui::GetStyle();
    // Play, Pause и Stop одинаковой ширины.
    constexpr float kButtonCount = 3.0f;
    const float buttonWidth = ImGui::CalcTextSize(icons::kPlay).x + style.FramePadding.x * 2.0f;
    const float width = buttonWidth * kButtonCount + style.ItemSpacing.x * (kButtonCount - 1.0f);
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - width) * 0.5f);

    const Vec4& accent = context.theme.colors().playMode;
    const bool playing = context.session.playing();
    if (playing) {
        ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPlay)) {
        context.actions.run(actions::kTogglePlay);
    }
    if (playing) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip(
        "%s (%s)",
        tr(playing ? "editor.play.stop" : "editor.play.play").c_str(),
        context.actions.shortcutText(actions::kTogglePlay).c_str()
    );

    ImGui::BeginDisabled(!playing);
    const bool paused = context.session.playMode().paused();
    if (paused) {
        ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(accent));
    }
    if (ImGui::SmallButton(icons::kPause)) {
        context.actions.run(actions::kPause);
    }
    if (paused) {
        ImGui::PopStyleColor();
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.pause").c_str());
    if (ImGui::SmallButton(icons::kStop)) {
        context.actions.run(actions::kStop);
    }
    ImGui::SetItemTooltip("%s", tr("editor.play.stop").c_str());
    ImGui::EndDisabled();
}

void drawLanguageMenu(const MainMenuContext& context)
{
    if (!ImGui::BeginMenu(tr("editor.menu.language").c_str())) {
        return;
    }
    for (const auto& [code, name] : context.languages) {
        const bool current = code == context.app.localization().language();
        if (ImGui::MenuItem(name.c_str(), nullptr, current) && !current) {
            context.app.setLanguage(code);
        }
    }
    ImGui::EndMenu();
}

void drawViewMenu(const MainMenuContext& context)
{
    if (!ImGui::BeginMenu(tr("editor.menu.view").c_str())) {
        return;
    }
    if (context.session.project()) {
        for (const auto& panel : context.panels) {
            if (panel->closable() && ImGui::MenuItem(tr(panel->titleKey()).c_str(), nullptr, panel->visible())) {
                panel->setVisible(!panel->visible());
            }
        }
        ImGui::Separator();
    }
    drawLanguageMenu(context);
    ImGui::EndMenu();
}

} // namespace

void drawMainMenu(const MainMenuContext& context)
{
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu(tr("editor.menu.file").c_str())) {
        if (context.session.project()) {
            context.actions.menuItem(actions::kSaveScene);
            context.actions.menuItem(actions::kCloseProject);
            ImGui::Separator();
        }
        context.actions.menuItem(actions::kQuit);
        ImGui::EndMenu();
    }
    if (context.session.project() && ImGui::BeginMenu(tr("editor.menu.edit").c_str())) {
        context.actions.menuItem(actions::kUndo);
        context.actions.menuItem(actions::kRedo);
        ImGui::EndMenu();
    }
    drawViewMenu(context);
    if (context.session.project()) {
        drawPlayControls(context);
    }
    ImGui::EndMainMenuBar();
}

} // namespace yqlora::editor
