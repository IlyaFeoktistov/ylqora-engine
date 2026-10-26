#include "ProjectManagerPanel.hpp"

#include "Icons.hpp"
#include "Theme.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/editor/ProjectList.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/platform/Platform.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace yqlora {

namespace {

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();

    return {text.begin(), text.end()};
}

std::filesystem::path fromUtf8(const std::string& text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

// Текст ошибки красным, если она есть.
void drawError(const std::string& errorKey)
{
    if (!errorKey.empty()) {
        const Vec4& color = theme::colors().errorText;
        ImGui::TextColored(ImVec4(color.r, color.g, color.b, color.a), "%s", tr(errorKey).c_str());
    }
}

} // namespace

ProjectManagerPanel::ProjectManagerPanel(ProjectList& projects, const SceneSerializer& serializer)
    : m_projects(projects)
    , m_serializer(serializer)
{
}

std::optional<Project> ProjectManagerPanel::draw(Application& app)
{
    m_toOpen.reset();
    if (m_refreshRequested) {
        m_refreshRequested = false;
        refresh();
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
                                   | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
    if (ImGui::Begin("###ProjectManager", nullptr, flags)) {
        ImGui::TextUnformatted(tr("projects.title").c_str());
        ImGui::Separator();

        if (ImGui::Button((std::string(icons::kAdd) + "  " + tr("projects.new")).c_str())) {
            m_newName.clear();
            if (m_location.empty()) {
                m_location = toUtf8(platform::documentsPath() / "Yqlora");
            }
            m_error.clear();
            m_openCreate = true;
        }
        ImGui::SameLine();
        if (ImGui::Button((std::string(icons::kImport) + "  " + tr("projects.import")).c_str())) {
            m_importPath.clear();
            m_error.clear();
            m_openImport = true;
        }
        ImGui::SameLine();
        const bool hasSelection = m_selected >= 0 && m_selected < static_cast<int>(m_rows.size());
        const bool canOpen = hasSelection && !m_rows[static_cast<std::size_t>(m_selected)].missing;
        ImGui::BeginDisabled(!canOpen);
        if (ImGui::Button((std::string(icons::kFolderOpen) + "  " + tr("projects.open")).c_str())) {
            open(m_rows[static_cast<std::size_t>(m_selected)]);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(!hasSelection);
        if (ImGui::Button((std::string(icons::kDelete) + "  " + tr("projects.remove")).c_str())) {
            m_deleteFolder = false;
            m_error.clear();
            m_openRemove = true;
        }
        ImGui::EndDisabled();

        ImGui::Spacing();
        drawList();

        drawCreateDialog(app);
        drawImportDialog(app);
        drawRemoveDialog();
    }
    ImGui::End();

    return m_toOpen;
}

void ProjectManagerPanel::refresh()
{
    m_rows.clear();
    for (const ProjectList::Entry& entry : m_projects.entries()) {
        Row row;
        row.directory = entry.directory;
        if (const auto project = Project::load(entry.directory)) {
            row.name = project->name;
        } else {
            row.name = toUtf8(entry.directory.filename());
            row.missing = true;
        }
        m_rows.push_back(std::move(row));
    }
    if (m_selected >= static_cast<int>(m_rows.size())) {
        m_selected = -1;
    }
}

void ProjectManagerPanel::drawList()
{
    if (m_rows.empty()) {
        ImGui::TextDisabled("%s", tr("projects.empty").c_str());

        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY;
    if (!ImGui::BeginTable("projects", 2, flags)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(tr("projects.column.name").c_str(), ImGuiTableColumnFlags_WidthStretch, 1.0f);
    ImGui::TableSetupColumn(tr("projects.column.path").c_str(), ImGuiTableColumnFlags_WidthStretch, 2.0f);
    ImGui::TableHeadersRow();

    for (std::size_t i = 0; i < m_rows.size(); ++i) {
        const Row& row = m_rows[i];
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::PushID(static_cast<int>(i));

        const bool selected = m_selected == static_cast<int>(i);
        const ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_SpanAllColumns
                                                     | ImGuiSelectableFlags_AllowDoubleClick;
        if (row.missing) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        }
        if (ImGui::Selectable(row.name.c_str(), selected, selectableFlags)) {
            m_selected = static_cast<int>(i);
            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && !row.missing) {
                open(row);
            }
        }
        ImGui::TableNextColumn();
        if (row.missing) {
            ImGui::Text("%s (%s)", toUtf8(row.directory).c_str(), tr("projects.missing").c_str());
            ImGui::PopStyleColor();
        } else {
            ImGui::TextUnformatted(toUtf8(row.directory).c_str());
        }

        ImGui::PopID();
    }
    ImGui::EndTable();
}

void ProjectManagerPanel::drawCreateDialog(Application& app)
{
    const std::string title = tr("projects.new") + "###CreateProject";
    if (m_openCreate) {
        ImGui::OpenPopup(title.c_str());
        m_openCreate = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::SetNextItemWidth(420.0f);
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::InputText(tr("projects.name").c_str(), &m_newName);
    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputText(tr("projects.location").c_str(), &m_location);
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.browse").c_str())) {
        app.showFolderDialog(fromUtf8(m_location), [this](std::optional<std::filesystem::path> folder) {
            if (folder) {
                m_location = toUtf8(*folder);
            }
        });
    }
    if (!m_newName.empty()) {
        ImGui::TextDisabled("%s", toUtf8(fromUtf8(m_location) / fromUtf8(m_newName)).c_str());
    }
    drawError(m_error);

    ImGui::Separator();
    if (ImGui::Button(tr("projects.create").c_str())) {
        if (auto project = Project::create(fromUtf8(m_location), m_newName, m_serializer, m_error)) {
            m_projects.add(project->directory);
            commit();
            m_toOpen = std::move(project);
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.cancel").c_str())) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ProjectManagerPanel::drawImportDialog(Application& app)
{
    const std::string title = tr("projects.import") + "###ImportProject";
    if (m_openImport) {
        ImGui::OpenPopup(title.c_str());
        m_openImport = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::SetNextItemWidth(420.0f);
    ImGui::InputText(tr("projects.importPath").c_str(), &m_importPath);
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.browse").c_str())) {
        app.showFolderDialog(platform::documentsPath(), [this](std::optional<std::filesystem::path> folder) {
            if (folder) {
                m_importPath = toUtf8(*folder);
            }
        });
    }
    drawError(m_error);

    ImGui::Separator();
    if (ImGui::Button(tr("projects.add").c_str())) {
        const std::filesystem::path directory = fromUtf8(m_importPath);
        if (Project::load(directory)) {
            m_projects.add(directory);
            commit();
            ImGui::CloseCurrentPopup();
        } else {
            m_error = "projects.error.notProject";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.cancel").c_str())) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ProjectManagerPanel::drawRemoveDialog()
{
    const std::string title = tr("projects.removeTitle") + "###RemoveProject";
    if (m_openRemove) {
        ImGui::OpenPopup(title.c_str());
        m_openRemove = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    if (m_selected < 0 || m_selected >= static_cast<int>(m_rows.size())) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();

        return;
    }
    const Row& row = m_rows[static_cast<std::size_t>(m_selected)];

    ImGui::TextUnformatted(tr("projects.removeQuestion").c_str());
    ImGui::TextDisabled("%s", toUtf8(row.directory).c_str());
    if (!row.missing) {
        ImGui::Checkbox(tr("projects.deleteFolder").c_str(), &m_deleteFolder);
        if (m_deleteFolder) {
            drawError("projects.deleteWarning");
        }
    }
    drawError(m_error);

    ImGui::Separator();
    if (ImGui::Button(tr("projects.remove").c_str())) {
        const bool removed = !m_deleteFolder || row.missing || Project::deleteFolder(row.directory, m_error);
        if (removed) {
            m_projects.remove(row.directory);
            m_selected = -1;
            commit();
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.cancel").c_str())) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ProjectManagerPanel::open(const Row& row)
{
    if (auto project = Project::load(row.directory)) {
        m_toOpen = std::move(project);
    } else {
        // Папку убрали, пока менеджер был открыт. Перечитываем список
        // в следующем кадре: сейчас по нему, возможно, идёт цикл.
        m_refreshRequested = true;
    }
}

void ProjectManagerPanel::commit()
{
    if (!m_projects.save()) {
        log::error("Cannot save the project list");
    }
    refresh();
}

} // namespace yqlora
