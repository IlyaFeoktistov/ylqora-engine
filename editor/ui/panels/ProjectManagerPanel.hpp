#pragma once

#include "yqlora/editor/ProjectManager.hpp"
#include "yqlora/project/Project.hpp"

#include <optional>
#include <string>

namespace yqlora {

class Application;

} // namespace yqlora

namespace yqlora::editor {

class Theme;

// Экран при запуске: список проектов, создание, добавление существующего,
// удаление. Занимает всё окно, пока проект не открыт. Только окна и поля
// ввода, сами действия - в ProjectManager.
class ProjectManagerPanel {
public:
    ProjectManagerPanel(ProjectManager& manager, const Theme& theme);

    // Возвращает проект, если в этом кадре его решили открыть.
    std::optional<Project> draw(Application& app);

private:
    void drawList();
    void drawCreateDialog(Application& app);
    void drawImportDialog(Application& app);
    void drawRemoveDialog();
    // Проект старого формата: спросить, переводить ли его на текущий.
    void drawUpgradeDialog();
    // Проект нельзя открыть: объяснить почему.
    void drawCannotOpenDialog();
    void open(std::size_t row);

    ProjectManager& m_manager;
    const Theme& m_theme;
    int m_selected = -1;
    std::optional<Project> m_toOpen;

    // Диалоги. Флаг открывает окно в следующем кадре: ImGui требует открывать
    // всплывающее окно на том же уровне стека, где оно рисуется.
    bool m_openCreate = false;
    bool m_openImport = false;
    bool m_openRemove = false;
    bool m_openUpgrade = false;
    bool m_openCannotOpen = false;
    // Проект, о котором спрашивает окно перевода или окно "нельзя открыть".
    ProjectManager::Row m_problemRow;
    std::string m_newName;
    std::string m_location;
    std::string m_importPath;
    bool m_deleteFolder = false;
    std::string m_error;
};

} // namespace yqlora::editor
