#include "yqlora/editor/ui/panels/LogPanel.hpp"

#include "yqlora/editor/ui/style/Icons.hpp"
#include "yqlora/editor/ui/style/Theme.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <array>
#include <string>

namespace yqlora::editor {

namespace {

constexpr std::array<log::Level, kLogLevelCount> kLevels{log::Level::Info, log::Level::Warning, log::Level::Error};

const char* iconOf(log::Level level)
{
    switch (level) {
    case log::Level::Info: return icons::kInfo;
    case log::Level::Warning: return icons::kWarning;
    case log::Level::Error: return icons::kError;
    }

    return icons::kInfo;
}

const char* levelKey(log::Level level)
{
    switch (level) {
    case log::Level::Info: return "editor.log.info";
    case log::Level::Warning: return "editor.log.warnings";
    case log::Level::Error: return "editor.log.errors";
    }

    return "editor.log.info";
}

ImVec4 colorOf(log::Level level, const EditorColors& colors)
{
    switch (level) {
    case log::Level::Info: return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    case log::Level::Warning: return toImVec4(colors.warningText);
    case log::Level::Error: return toImVec4(colors.errorText);
    }

    return ImGui::GetStyleColorVec4(ImGuiCol_Text);
}

} // namespace

void LogPanel::draw(EditorContext& context)
{
    if (m_focusRequested) {
        ImGui::SetNextWindowFocus();
        m_focusRequested = false;
    }
    if (!beginWindow(context)) {
        ImGui::End();

        return;
    }
    const EditorColors& colors = context.theme.colors();
    drawToolbar(colors);
    ImGui::Separator();
    drawEntries(colors);
    ImGui::End();
}

void LogPanel::drawToolbar(const EditorColors& colors)
{
    if (ImGui::Button(icons::kClear)) {
        m_buffer.clear();
    }
    ImGui::SetItemTooltip("%s", tr("editor.log.clear").c_str());
    ImGui::SameLine();
    if (ImGui::Button(icons::kCopy)) {
        std::string text;
        for (const std::size_t index : m_shown) {
            text += m_buffer.entries()[index].message;
            text += '\n';
        }
        ImGui::SetClipboardText(text.c_str());
    }
    ImGui::SetItemTooltip("%s", tr("editor.log.copyAll").c_str());

    for (const log::Level level : kLevels) {
        ImGui::SameLine();
        bool& shown = m_filter.levels[static_cast<std::size_t>(level)];
        const std::string label = std::string(iconOf(level)) + " " + std::to_string(m_buffer.count(level)) + "##"
                                  + levelKey(level);
        ImGui::PushStyleColor(
            ImGuiCol_Text,
            shown ? colorOf(level, colors) : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled)
        );
        if (ImGui::Button(label.c_str())) {
            shown = !shown;
        }
        ImGui::PopStyleColor();
        ImGui::SetItemTooltip("%s", tr(levelKey(level)).c_str());
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    const std::string hint = std::string(icons::kSearch) + " " + tr("editor.log.search");
    ImGui::InputTextWithHint("##search", hint.c_str(), &m_filter.text);
}

void LogPanel::drawEntries(const EditorColors& colors)
{
    m_shown.clear();
    const auto& entries = m_buffer.entries();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        if (m_filter.matches(entries[i])) {
            m_shown.push_back(i);
        }
    }

    if (!ImGui::BeginChild(
            "##entries",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_None,
            ImGuiWindowFlags_HorizontalScrollbar
        )) {
        ImGui::EndChild();

        return;
    }
    // Был внизу до новых записей - остаётся внизу; прокрутили вверх - не мешаем читать.
    const bool followEnd = ImGui::GetScrollY() >= ImGui::GetScrollMaxY();

    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(m_shown.size()));
    while (clipper.Step()) {
        for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
            const LogEntry& entry = entries[m_shown[static_cast<std::size_t>(row)]];
            ImGui::PushID(row);
            ImGui::PushStyleColor(ImGuiCol_Text, colorOf(entry.level, colors));
            ImGui::TextUnformatted(iconOf(entry.level));
            ImGui::SameLine();
            ImGui::TextUnformatted(entry.message.c_str());
            ImGui::PopStyleColor();
            if (ImGui::BeginPopupContextItem("##entry")) {
                if (ImGui::MenuItem(tr("editor.log.copy").c_str())) {
                    ImGui::SetClipboardText(entry.message.c_str());
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
        }
    }

    if (followEnd && m_buffer.received() != m_seenReceived) {
        ImGui::SetScrollHereY(1.0f);
    }
    m_seenReceived = m_buffer.received();
    ImGui::EndChild();
}

} // namespace yqlora::editor
