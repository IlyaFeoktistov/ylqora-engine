#include "dialogs/UnsavedChangesDialog.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"

#include <imgui.h>

#include <string>
#include <utility>

namespace yqlora::editor {

void UnsavedChangesDialog::request(const EditorSession& session, std::function<void()> then)
{
    if (!session.document().modified()) {
        then();

        return;
    }
    m_then = std::move(then);
    m_openRequested = true;
}

void UnsavedChangesDialog::draw(EditorSession& session)
{
    const std::string title = tr("editor.unsaved.title") + "###Unsaved";
    if (m_openRequested) {
        ImGui::OpenPopup(title.c_str());
        m_openRequested = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::TextUnformatted(tr("editor.unsaved.question").c_str());
    ImGui::Separator();

    if (ImGui::Button(tr("editor.unsaved.save").c_str())) {
        // Не сохранилось - остаёмся, чтобы изменения не пропали.
        if (session.saveScene()) {
            std::exchange(m_then, nullptr)();
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.unsaved.discard").c_str())) {
        std::exchange(m_then, nullptr)();
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.common.cancel").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        m_then = nullptr;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

} // namespace yqlora::editor
