#pragma once

#include "yqlora/editor/ui/core/EditorPanel.hpp"

#include "yqlora/editor/ProjectFiles.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace yqlora {

class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

class EditorSession;
struct EditorColors;

// Файлы проекта внизу редактора: сцены и папки. Двойной клик по сцене
// открывает её, правый клик - меню с остальными действиями.
class FileSystemPanel final : public EditorPanel {
public:
    const char* id() const override
    {
        return "FileSystem";
    }

    const char* titleKey() const override
    {
        return "editor.window.files";
    }

    void draw(EditorContext& context) override;

    void onProjectOpened(EditorContext& /*context*/) override
    {
        m_dirty = true;
    }

private:
    void drawEntry(
        const ProjectFiles::Entry& entry,
        EditorSession& session,
        const SceneSerializer& serializer,
        const EditorColors& colors
    );
    void drawContextMenu(const ProjectFiles::Entry& entry, EditorSession& session, const SceneSerializer& serializer);
    void drawRenameDialog(EditorSession& session, const EditorColors& colors);
    void drawDeleteDialog(EditorSession& session);
    // Папка, куда класть новые файлы: выбранная папка или папка выбранного файла.
    std::filesystem::path targetFolder() const;
    void startRename(const std::filesystem::path& path);

    ProjectFiles::Entry m_root;
    std::filesystem::path m_projectDirectory;
    std::filesystem::path m_selected;
    bool m_selectedIsDirectory = false;
    bool m_dirty = true;
    bool m_wasFocused = false;
    std::optional<std::filesystem::path> m_openRequest;

    bool m_renameRequested = false;
    std::filesystem::path m_renaming;
    std::string m_newName;
    bool m_deleteRequested = false;
    std::filesystem::path m_deleting;
    std::string m_error;
};

} // namespace yqlora::editor
