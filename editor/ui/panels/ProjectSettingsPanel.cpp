#include "panels/ProjectSettingsPanel.hpp"

#include "style/Theme.hpp"
#include "widgets/PropertyTable.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <imgui.h>

#include <string>

namespace yqlora::editor {

namespace {

// Окно открывают поверх остальных, поэтому размер задаётся при первом показе.
constexpr ImVec2 kFirstSize{720.0f, 440.0f};
constexpr float kSectionListWidth = 180.0f;

// Ключ перевода имени раздела: editor.settings.<имя типа>.
std::string sectionTitle(const SettingsSection& section)
{
    return tr("editor.settings." + std::string(section.typeName()));
}

} // namespace

void ProjectSettingsPanel::draw(EditorContext& context)
{
    EditorSession& session = context.session;
    if (m_focusRequested) {
        ImGui::SetNextWindowFocus();
        m_focusRequested = false;
    }
    ImGui::SetNextWindowSize(kFirstSize, ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(title().c_str(), visibleFlag())) {
        ImGui::End();

        return;
    }
    if (!session.project() || session.project()->settings().sections().empty()) {
        ImGui::TextDisabled("%s", tr("editor.settings.nothing").c_str());
        ImGui::End();

        return;
    }

    // Версия формата не настройка: её меняет только перевод проекта, поэтому она лишь текстом.
    ImGui::TextDisabled("%s %d", tr("editor.settings.projectVersion").c_str(), Project::formatVersion());
    ImGui::SetItemTooltip("%s", tr("editor.settings.projectVersionHint").c_str());

    ProjectSettings& settings = session.project()->settings();
    SettingsSection* selected = settings.find(m_selected);
    if (!selected) {
        selected = settings.sections().front().get();
    }

    ImGui::BeginChild("sections", ImVec2(kSectionListWidth, 0.0f), ImGuiChildFlags_Borders);
    for (const auto& section : settings.sections()) {
        ImGui::PushID(section.get());
        if (ImGui::Selectable(sectionTitle(*section).c_str(), section.get() == selected)) {
            selected = section.get();
        }
        ImGui::PopID();
    }
    ImGui::EndChild();
    m_selected = std::string(selected->typeName());

    ImGui::SameLine();
    ImGui::BeginChild("properties");
    bool dragging = false;
    if (const TypeInfo* type = session.registry().find(selected->typeName())) {
        const PropertyEditorContext widgets{
            .colors = context.theme.colors(),
            .metrics = context.theme.metrics(),
            .scene = session.scene(),
            .resources = &session.project()->resources(),
        };
        dragging = drawPropertyTable(
            session.registry(),
            widgets,
            *type,
            *selected,
            [&session, selected](const Property& property, const PropertyValue& value) {
                session.settingsEditing().setProperty(*selected, property, value);
            }
        );
    }
    ImGui::EndChild();
    ImGui::End();
    context.mouseWrapping = context.mouseWrapping || dragging;
}

} // namespace yqlora::editor
