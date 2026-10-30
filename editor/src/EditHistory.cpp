#include "yqlora/editor/EditHistory.hpp"

#include <algorithm>

namespace yqlora::editor {

bool EditHistory::execute(EditorSession& session, std::unique_ptr<Command> command)
{
    if (!command || !command->apply(session)) {
        return false;
    }
    if (m_editOpen && canUndo() && !canRedo() && m_commands.back()->merge(*command)) {
        // Последняя команда изменилась: сохранённое после неё состояние уже не то.
        if (m_saved == m_position && m_commands.back()->changesScene()) {
            m_saved.reset();
        }

        return true;
    }

    // Отброшенные отменённые команды не меняли сцену - сохранённое
    // состояние и есть текущее, иначе до него уже не дойти.
    if (m_saved && *m_saved > m_position) {
        m_saved = sceneUnchanged(m_position, *m_saved) ? std::optional(m_position) : std::nullopt;
    }
    m_commands.resize(m_position);
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

bool EditHistory::atSaved() const
{
    return m_saved && sceneUnchanged(std::min(*m_saved, m_position), std::max(*m_saved, m_position));
}

bool EditHistory::sceneUnchanged(std::size_t from, std::size_t to) const
{
    return std::none_of(m_commands.begin() + from, m_commands.begin() + to, [](const auto& command) {
        return command->changesScene();
    });
}

} // namespace yqlora::editor
