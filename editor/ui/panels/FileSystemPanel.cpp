#include "panels/FileSystemPanel.hpp"

#include "style/Icons.hpp"
#include "style/Theme.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>

#include <utility>

namespace yqlora::editor {

namespace {

std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();

    return {text.begin(), text.end()};
}

const char* iconFor(const ProjectFiles::Entry& entry, bool open)
{
    if (entry.directory) {
        return open ? icons::kFolderOpen : icons::kFolder;
    }

    return ProjectFiles::isScene(entry.path) ? icons::kScene : icons::kFile;
}

// Кнопка с иконкой; текст - во всплывающей подсказке.
bool iconButton(const char* icon, const std::string& hint)
{
    const bool pressed = ImGui::Button(icon);
    ImGui::SetItemTooltip("%s", hint.c_str());

    return pressed;
}

} // namespace

void FileSystemPanel::draw(EditorContext& context)
{
    EditorSession& session = context.session;
    const SceneSerializer& serializer = context.serializer;
    if (!ImGui::Begin(title().c_str(), visibleFlag()) || !session.project()) {
        ImGui::End();

        return;
    }
    Project& project = *session.project();
    ProjectFiles files(project);

    // Перечитываем, когда открыли другой проект и когда панель получила фокус:
    // файлы могли поменять в проводнике.
    const bool focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    if (project.directory() != m_projectDirectory || (focused && !m_wasFocused)) {
        m_projectDirectory = project.directory();
        m_dirty = true;
    }
    m_wasFocused = focused;
    if (m_dirty) {
        m_root = files.scan();
        m_dirty = false;
    }

    if (iconButton(icons::kAddFile, tr("editor.files.newScene"))) {
        if (const auto created = files.createScene(targetFolder(), serializer)) {
            m_dirty = true;
            startRename(*created);
        }
    }
    ImGui::SameLine();
    if (iconButton(icons::kAddFolder, tr("editor.files.newFolder"))) {
        if (const auto created = files.createFolder(targetFolder())) {
            m_dirty = true;
            startRename(*created);
        }
    }
    ImGui::SameLine();
    if (iconButton(icons::kRefresh, tr("editor.files.refresh"))) {
        m_dirty = true;
    }
    ImGui::Separator();

    if (ImGui::BeginChild("files")) {
        for (const auto& entry : m_root.children) {
            drawEntry(entry, session, serializer, context.theme.colors());
        }
        if (m_root.children.empty()) {
            ImGui::TextDisabled("%s", tr("editor.files.empty").c_str());
        }
    }
    ImGui::EndChild();

    drawRenameDialog(session, context.theme.colors());
    drawDeleteDialog(session);
    ImGui::End();

    if (m_openRequest) {
        context.openScene(*std::exchange(m_openRequest, std::nullopt));
    }
}

void FileSystemPanel::drawEntry(
    const ProjectFiles::Entry& entry,
    EditorSession& session,
    const SceneSerializer& serializer,
    const EditorColors& colors
)
{
    const bool selected = entry.path == m_selected;
    const bool openScene = entry.path == session.scenePath();
    const auto mainScenePath = session.project()->mainScenePath();
    const bool mainScene = mainScenePath && entry.path == *mainScenePath;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick
                               | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!entry.directory) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }
    if (selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    // Открытая сцена выделена цветом, чтобы было видно, что сейчас правится.
    if (openScene) {
        ImGui::PushStyleColor(ImGuiCol_Text, toImVec4(colors.selectionOutline));
    }
    const std::string id = toUtf8(entry.path);
    // Иконка папки зависит от того, раскрыта ли она, а это известно до отрисовки узла.
    const bool expanded = entry.directory && ImGui::TreeNodeGetOpen(ImGui::GetID(id.c_str()));
    const bool open = ImGui::TreeNodeEx(id.c_str(), flags, "%s  %s", iconFor(entry, expanded), entry.name.c_str());
    if (openScene) {
        ImGui::PopStyleColor();
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) || ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        m_selected = entry.path;
        m_selectedIsDirectory = entry.directory;
    }
    if (!entry.directory && ProjectFiles::isScene(entry.path) && ImGui::IsItemHovered()
        && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
        m_openRequest = entry.path;
    }
    // Меню привязывается к последнему элементу, поэтому сразу после строки файла.
    drawContextMenu(entry, session, serializer);
    if (mainScene) {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", icons::kMainScene);
        ImGui::SetItemTooltip("%s", tr("editor.files.mainScene").c_str());
    }

    if (entry.directory && open) {
        for (const auto& child : entry.children) {
            drawEntry(child, session, serializer, colors);
        }
        ImGui::TreePop();
    }
}

void FileSystemPanel::drawContextMenu(
    const ProjectFiles::Entry& entry,
    EditorSession& session,
    const SceneSerializer& serializer
)
{
    if (!ImGui::BeginPopupContextItem()) {
        return;
    }
    ProjectFiles files(*session.project());
    const auto mainScene = session.project()->mainScenePath();
    const auto item = [](const char* icon, const char* key) {
        const std::string label = std::string(icon) + "  " + tr(key);

        return ImGui::MenuItem(label.c_str());
    };

    if (entry.directory) {
        if (item(icons::kAddFile, "editor.files.newScene")) {
            if (const auto created = files.createScene(entry.path, serializer)) {
                m_dirty = true;
                startRename(*created);
            }
        }
        if (item(icons::kAddFolder, "editor.files.newFolder")) {
            if (const auto created = files.createFolder(entry.path)) {
                m_dirty = true;
                startRename(*created);
            }
        }
        ImGui::Separator();
    } else if (ProjectFiles::isScene(entry.path)) {
        if (item(icons::kScene, "editor.files.open")) {
            m_openRequest = entry.path;
        }
        if (entry.path != mainScene && item(icons::kMainScene, "editor.files.setMain")) {
            session.setMainScene(entry.path);
        }
        ImGui::Separator();
    }
    if (item(icons::kRename, "editor.files.rename")) {
        startRename(entry.path);
    }
    if (item(icons::kDelete, "editor.files.delete")) {
        m_deleting = entry.path;
        m_deleteRequested = true;
    }
    ImGui::EndPopup();
}

void FileSystemPanel::drawRenameDialog(EditorSession& session, const EditorColors& colors)
{
    const std::string title = tr("editor.files.rename") + "###RenameFile";
    if (m_renameRequested) {
        ImGui::OpenPopup(title.c_str());
        m_renameRequested = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SetNextItemWidth(360.0f);
    const bool entered = ImGui::InputText("##name", &m_newName, ImGuiInputTextFlags_EnterReturnsTrue);
    if (!m_error.empty()) {
        ImGui::TextColored(toImVec4(colors.errorText), "%s", tr(m_error).c_str());
    }

    if (ImGui::Button(tr("editor.files.rename").c_str()) || entered) {
        if (const auto renamed = ProjectFiles(*session.project()).rename(m_renaming, m_newName)) {
            if (m_selected == m_renaming) {
                m_selected = *renamed;
            }
            m_dirty = true;
            ImGui::CloseCurrentPopup();
        } else {
            m_error = "editor.files.error.rename";
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.common.cancel").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void FileSystemPanel::drawDeleteDialog(EditorSession& session)
{
    const std::string title = tr("editor.files.delete") + "###DeleteFile";
    if (m_deleteRequested) {
        ImGui::OpenPopup(title.c_str());
        m_deleteRequested = false;
    }
    if (!ImGui::BeginPopupModal(title.c_str(), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    ImGui::TextUnformatted(tr("editor.files.deleteQuestion").c_str());
    ImGui::TextDisabled("%s", toUtf8(m_deleting).c_str());
    ImGui::Separator();

    if (ImGui::Button(tr("editor.files.delete").c_str())) {
        ProjectFiles(*session.project()).remove(m_deleting);
        if (m_selected == m_deleting) {
            m_selected.clear();
        }
        m_dirty = true;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.common.cancel").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

std::filesystem::path FileSystemPanel::targetFolder() const
{
    if (m_selected.empty()) {
        return {};
    }

    return m_selectedIsDirectory ? m_selected : m_selected.parent_path();
}

void FileSystemPanel::startRename(const std::filesystem::path& path)
{
    m_renaming = path;
    m_newName = toUtf8(path.filename());
    m_error.clear();
    m_renameRequested = true;
}

} // namespace yqlora::editor
