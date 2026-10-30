#pragma once

#include "yqlora/editor/Command.hpp"

#include <cstddef>
#include <memory>
#include <vector>

namespace yqlora::editor {

// История правок сцены и настроек проекта для отмены и повтора. Сохранено
// ли что-то, она не решает: сцена и настройки сравнивают себя со своими
// файлами сами (SceneDocument::modified, ProjectSettings::modified), поэтому
// правка в обход истории тоже видна.
class EditHistory {
public:
    // Отменённые команды после текущего места отбрасываются. Пока не вызван
    // finishEdit, команда может слиться с предыдущей.
    bool execute(EditorSession& session, std::unique_ptr<Command> command);

    void finishEdit()
    {
        m_editOpen = false;
    }

    bool canUndo() const
    {
        return m_position > 0;
    }

    bool canRedo() const
    {
        return m_position < m_commands.size();
    }

    bool undo(EditorSession& session);
    bool redo(EditorSession& session);

    // Забывает все команды.
    void clear();

private:
    std::vector<std::unique_ptr<Command>> m_commands;
    // Команды с этого места отменены.
    std::size_t m_position = 0;
    bool m_editOpen = false;
};

} // namespace yqlora::editor
