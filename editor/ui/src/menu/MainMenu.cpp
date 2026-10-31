#include "yqlora/editor/ui/menu/MainMenu.hpp"

#include "yqlora/editor/ui/core/ActionIds.hpp"
#include "yqlora/editor/ui/core/Actions.hpp"
#include "yqlora/editor/ui/core/EditorPanel.hpp"
#include "yqlora/editor/ui/style/Icons.hpp"
#include "yqlora/editor/ui/style/Theme.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/LogBuffer.hpp"
#include "yqlora/editor/ui/core/EditorHost.hpp"
#include "yqlora/i18n/Language.hpp"
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

// Состояние сборки скриптов, ошибки и предупреждения лога у правого края;
// щелчок открывает лог.
void drawStatus(const MainMenuContext& context)
{
    const std::string errors = std::string(icons::kError) + " " + std::to_string(context.log.count(log::Level::Error));
    const std::string warnings = std::string(icons::kWarning) + " "
                                 + std::to_string(context.log.count(log::Level::Warning));
    std::string build;
    if (context.buildingScripts) {
        build = std::string(icons::kBuild) + " " + tr("editor.scripts.building");
    } else if (context.scriptsFailed) {
        build = std::string(icons::kError) + " " + tr("editor.scripts.failed");
    }
    const ImGuiStyle& style = ImGui::GetStyle();
    float width = ImGui::CalcTextSize(errors.c_str()).x + ImGui::CalcTextSize(warnings.c_str()).x
                  + style.FramePadding.x * 4.0f + style.ItemSpacing.x * 2.0f;
    if (!build.empty()) {
        width += ImGui::CalcTextSize(build.c_str()).x + style.FramePadding.x * 2.0f + style.ItemSpacing.x;
    }
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - width);

    const EditorColors& colors = context.theme.colors();
    if (context.buildingScripts) {
        ImGui::TextDisabled("%s", build.c_str());
        ImGui::SameLine();
    } else if (context.scriptsFailed) {
        ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(colors.errorText));
        if (ImGui::SmallButton(build.c_str())) {
            context.showLog();
        }
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("%s", tr("editor.scripts.failedTooltip").c_str());
        ImGui::SameLine();
    }
    const auto counter = [&](const std::string& label, std::size_t count, const Vec4& color) {
        if (count > 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(color));
        }
        if (ImGui::SmallButton(label.c_str())) {
            context.showLog();
        }
        if (count > 0) {
            ImGui::PopStyleColor();
        }
        ImGui::SetItemTooltip("%s", tr("editor.log.open").c_str());
    };
    counter(errors, context.log.count(log::Level::Error), colors.errorText);
    counter(warnings, context.log.count(log::Level::Warning), colors.warningText);
}

void drawLanguageMenu(const MainMenuContext& context)
{
    if (!ImGui::BeginMenu(tr("editor.menu.language").c_str())) {
        return;
    }
    for (const auto& [code, name] : context.languages) {
        const bool current = code == context.host.language().localization().language();
        if (ImGui::MenuItem(name.c_str(), nullptr, current) && !current) {
            context.host.language().set(code);
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
        context.actions.menuItem(actions::kMaximizePanel);
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
            context.actions.menuItem(actions::kSave);
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
    if (context.session.project() && ImGui::BeginMenu(tr("editor.menu.project").c_str())) {
        context.actions.menuItem(actions::kProjectSettings);
        ImGui::EndMenu();
    }
    if (context.session.project() && ImGui::BeginMenu(tr("editor.menu.scripts").c_str())) {
        context.actions.menuItem(actions::kBuildScripts);
        ImGui::EndMenu();
    }
    drawViewMenu(context);
    if (context.session.project()) {
        drawPlayControls(context);
        ImGui::SameLine();
        drawStatus(context);
    }
    ImGui::EndMainMenuBar();
}

} // namespace yqlora::editor
