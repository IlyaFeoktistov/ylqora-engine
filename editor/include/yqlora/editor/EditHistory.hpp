#pragma once

#include "yqlora/editor/Command.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace yqlora {

// История правок сцены для отмены и повтора. Помнит, в каком месте истории
// сцена сохранена: по нему видно, есть ли несохранённые изменения.
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

    // Забывает все команды, нынешнее состояние считается сохранённым.
    void clear();

    void markSaved()
    {
        m_saved = m_position;
    }

    bool atSaved() const
    {
        return m_saved == m_position;
    }

private:
    std::vector<std::unique_ptr<Command>> m_commands;
    // Команды с этого места отменены.
    std::size_t m_position = 0;
    // nullopt, если до сохранённого состояния уже не дойти.
    std::optional<std::size_t> m_saved = 0;
    bool m_editOpen = false;
};

} // namespace yqlora
