#pragma once

#include "yqlora/editor/Project.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace yqlora {

class Application;
class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

class ProjectList;
class Theme;

// Экран при запуске: список проектов, создание, добавление существующего,
// удаление. Занимает всё окно, пока проект не открыт.
class ProjectManagerPanel {
public:
    ProjectManagerPanel(ProjectList& projects, const SceneSerializer& serializer, const Theme& theme);

    // Возвращает проект, если в этом кадре его решили открыть.
    std::optional<Project> draw(Application& app);

    // Перечитывает имена проектов из их папок.
    void refresh();

private:
    struct Row {
        std::filesystem::path directory;
        std::string name;
        bool missing = false;
    };

    void drawList();
    void drawCreateDialog(Application& app);
    void drawImportDialog(Application& app);
    void drawRemoveDialog();
    void open(const Row& row);
    // Сохраняет список и перечитывает строки.
    void commit();

    ProjectList& m_projects;
    const SceneSerializer& m_serializer;
    const Theme& m_theme;
    std::vector<Row> m_rows;
    int m_selected = -1;
    std::optional<Project> m_toOpen;
    bool m_refreshRequested = false;

    // Диалоги. Флаг открывает окно в следующем кадре: ImGui требует открывать
    // всплывающее окно на том же уровне стека, где оно рисуется.
    bool m_openCreate = false;
    bool m_openImport = false;
    bool m_openRemove = false;
    std::string m_newName;
    std::string m_location;
    std::string m_importPath;
    bool m_deleteFolder = false;
    std::string m_error;
};

} // namespace yqlora::editor
