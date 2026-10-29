#include "panels/ProjectManagerPanel.hpp"

#include "style/Icons.hpp"
#include "style/Theme.hpp"
#include "widgets/Checkbox.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/core/Version.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/platform/Platform.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

namespace yqlora::editor {

namespace {

constexpr float kFieldWidth = 420.0f;
// Колонки списка: имя, версия и путь; пути вдвое больше места, чем имени.
constexpr int kColumnCount = 3;
constexpr float kNameColumnWeight = 1.0f;
constexpr float kPathColumnWeight = 2.0f;

// Текст ошибки красным, если она есть.
void drawError(const std::string& errorKey, const EditorColors& colors)
{
    if (!errorKey.empty()) {
        ImGui::TextColored(toImVec4(colors.errorText), "%s", tr(errorKey).c_str());
    }
}

// Версия движка проекта или прочерк, если она не записана.
std::string versionLabel(const std::string& version)
{
    return version.empty() ? "-" : version;
}

} // namespace

ProjectManagerPanel::ProjectManagerPanel(ProjectManager& manager, const Theme& theme)
    : m_manager(manager)
    , m_theme(theme)
{
}

std::optional<Project> ProjectManagerPanel::draw(Application& app)
{
    m_toOpen.reset();
    const auto rows = m_manager.rows();
    if (m_selected >= static_cast<int>(rows.size())) {
        m_selected = -1;
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
                m_location = toUtf8(platform::documentsPath() / Application::organization());
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
        const bool hasSelection = m_selected >= 0;
        const bool canOpen = hasSelection && !rows[static_cast<std::size_t>(m_selected)].missing;
        ImGui::BeginDisabled(!canOpen);
        if (ImGui::Button((std::string(icons::kFolderOpen) + "  " + tr("projects.open")).c_str())) {
            open(static_cast<std::size_t>(m_selected));
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
        ImGui::TextDisabled("%s %s", tr("projects.engineVersion").c_str(), std::string(engineVersion()).c_str());

        drawCreateDialog(app);
        drawImportDialog(app);
        drawRemoveDialog();
        drawUpgradeDialog();
        drawCannotOpenDialog();
    }
    ImGui::End();

    return m_toOpen;
}

void ProjectManagerPanel::drawList()
{
    const auto rows = m_manager.rows();
    if (rows.empty()) {
        ImGui::TextDisabled("%s", tr("projects.empty").c_str());

        return;
    }

    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY;
    if (!ImGui::BeginTable("projects", kColumnCount, flags)) {
        return;
    }
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(tr("projects.column.name").c_str(), ImGuiTableColumnFlags_WidthStretch, kNameColumnWeight);
    ImGui::TableSetupColumn(tr("projects.column.version").c_str(), ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn(tr("projects.column.path").c_str(), ImGuiTableColumnFlags_WidthStretch, kPathColumnWeight);
    ImGui::TableHeadersRow();

    for (std::size_t i = 0; i < rows.size(); ++i) {
        const ProjectManager::Row& row = rows[i];
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
                open(i);
            }
        }
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(versionLabel(row.engineVersion).c_str());
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

    ImGui::SetNextItemWidth(kFieldWidth);
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::InputText(tr("projects.name").c_str(), &m_newName);
    ImGui::SetNextItemWidth(kFieldWidth);
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
    drawError(m_error, m_theme.colors());

    ImGui::Separator();
    if (ImGui::Button(tr("projects.create").c_str())) {
        if (auto project = m_manager.create(fromUtf8(m_location), m_newName, m_error)) {
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

    ImGui::SetNextItemWidth(kFieldWidth);
    ImGui::InputText(tr("projects.importPath").c_str(), &m_importPath);
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.browse").c_str())) {
        app.showFolderDialog(platform::documentsPath(), [this](std::optional<std::filesystem::path> folder) {
            if (folder) {
                m_importPath = toUtf8(*folder);
            }
        });
    }
    drawError(m_error, m_theme.colors());

    ImGui::Separator();
    if (ImGui::Button(tr("projects.add").c_str())) {
        if (m_manager.import(fromUtf8(m_importPath), m_error)) {
            ImGui::CloseCurrentPopup();
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
    if (m_selected < 0 || m_selected >= static_cast<int>(m_manager.rows().size())) {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();

        return;
    }
    const ProjectManager::Row& row = m_manager.rows()[static_cast<std::size_t>(m_selected)];

    ImGui::TextUnformatted(tr("projects.removeQuestion").c_str());
    ImGui::TextDisabled("%s", toUtf8(row.directory).c_str());
    if (!row.missing) {
        checkbox(tr("projects.deleteFolder").c_str(), &m_deleteFolder, m_theme.metrics().checkboxPadding);
        if (m_deleteFolder) {
            drawError("projects.deleteWarning", m_theme.colors());
        }
    }
    drawError(m_error, m_theme.colors());

    ImGui::Separator();
    if (ImGui::Button(tr("projects.remove").c_str())) {
        if (m_manager.remove(static_cast<std::size_t>(m_selected), m_deleteFolder, m_error)) {
            m_selected = -1;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("projects.cancel").c_str())) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ProjectManagerPanel::drawUpgradeDialog()
{
    const std::string title = tr("projects.upgrade.title") + "###UpgradeProject";
    if (m_openUpgrade) {
        ImGui::OpenPopup(title.c_str());
        m_openUpgrade = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::TextUnformatted(m_problemRow.name.c_str());
    ImGui::Text("%s %s", tr("projects.upgrade.from").c_str(), versionLabel(m_problemRow.engineVersion).c_str());
    ImGui::Text("%s %s", tr("projects.upgrade.to").c_str(), std::string(engineVersion()).c_str());
    ImGui::Spacing();
    ImGui::TextUnformatted(tr("projects.upgrade.question").c_str());
    drawError("projects.upgrade.warning", m_theme.colors());
    drawError(m_error, m_theme.colors());

    ImGui::Separator();
    if (ImGui::Button(tr("projects.upgrade.confirm").c_str())) {
        if (auto project = m_manager.upgrade(m_problemRow.directory, m_error)) {
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

void ProjectManagerPanel::drawCannotOpenDialog()
{
    const std::string title = tr("projects.cannotOpen") + "###CannotOpenProject";
    if (m_openCannotOpen) {
        ImGui::OpenPopup(title.c_str());
        m_openCannotOpen = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    const bool newer = m_problemRow.compatibility == FormatCompatibility::Newer;
    ImGui::TextUnformatted(m_problemRow.name.c_str());
    ImGui::Text("%s %s", tr("projects.upgrade.from").c_str(), versionLabel(m_problemRow.engineVersion).c_str());
    ImGui::Text("%s %s", tr("projects.engineVersion").c_str(), std::string(engineVersion()).c_str());
    ImGui::Spacing();
    drawError(newer ? "projects.error.newer" : "projects.error.unsupported", m_theme.colors());

    ImGui::Separator();
    if (ImGui::Button(tr("projects.close").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void ProjectManagerPanel::open(std::size_t row)
{
    m_error.clear();
    auto result = m_manager.open(row);
    switch (result.status) {
    case ProjectManager::OpenStatus::Opened: m_toOpen = std::move(result.project); break;
    case ProjectManager::OpenStatus::NeedsUpgrade:
        m_problemRow = m_manager.rows()[row];
        m_openUpgrade = true;
        break;
    case ProjectManager::OpenStatus::CannotOpen:
        m_problemRow = m_manager.rows()[row];
        m_openCannotOpen = true;
        break;
    case ProjectManager::OpenStatus::Missing: break;
    }
}

} // namespace yqlora::editor
