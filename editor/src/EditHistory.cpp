#include "yqlora/editor/EditHistory.hpp"

namespace yqlora::editor {

bool EditHistory::execute(EditorSession& session, std::unique_ptr<Command> command)
{
    if (!command || !command->apply(session)) {
        return false;
    }
    if (m_editOpen && canUndo() && !canRedo() && m_commands.back()->merge(*command)) {
        // Последняя команда изменилась: сохранённое после неё состояние уже не то.
        if (m_saved == m_position) {
            m_saved.reset();
        }

        return true;
    }

    m_commands.resize(m_position);
    if (m_saved && *m_saved > m_position) {
        m_saved.reset();
    }
    m_commands.push_back(std::move(command));
    m_position = m_commands.size();
    m_editOpen = true;

    return true;
}

bool EditHistory::undo(EditorSession& session)
{
    if (!canUndo()) {
        return false;
    }
    m_editOpen = false;
    --m_position;
    m_commands[m_position]->revert(session);

    return true;
}

bool EditHistory::redo(EditorSession& session)
{
    if (!canRedo()) {
        return false;
    }
    m_editOpen = false;
    // Повтор не удался: история остаётся на месте.
    if (!m_commands[m_position]->apply(session)) {
        return false;
    }
    ++m_position;

    return true;
}

void EditHistory::clear()
{
    m_commands.clear();
    m_position = 0;
    m_saved = 0;
    m_editOpen = false;
}

} // namespace yqlora::editor
