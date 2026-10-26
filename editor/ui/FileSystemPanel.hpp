#pragma once

#include "yqlora/editor/ProjectFiles.hpp"

#include <filesystem>
#include <optional>
#include <string>

namespace yqlora {

class EditorSession;
class SceneSerializer;

// Файлы проекта внизу редактора: сцены и папки. Двойной клик по сцене
// открывает её, правый клик - меню с остальными действиями.
class FileSystemPanel {
public:
    void draw(EditorSession& session, const SceneSerializer& serializer);

    // Сцена, которую попросили открыть. Открывает её приложение: сначала
    // оно спросит про несохранённые изменения в текущей.
    std::optional<std::filesystem::path> takeOpenRequest();

    // Файлы поменялись не через панель: перечитать при следующей отрисовке.
    void refresh()
    {
        m_dirty = true;
    }

private:
    void drawEntry(const projectfiles::Entry& entry, EditorSession& session, const SceneSerializer& serializer);
    void drawContextMenu(const projectfiles::Entry& entry, EditorSession& session, const SceneSerializer& serializer);
    void drawRenameDialog(EditorSession& session);
    void drawDeleteDialog(EditorSession& session);
    // Папка, куда класть новые файлы: выбранная папка или папка выбранного файла.
    std::filesystem::path targetFolder() const;
    void startRename(const std::filesystem::path& path);

    projectfiles::Entry m_root;
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

} // namespace yqlora
