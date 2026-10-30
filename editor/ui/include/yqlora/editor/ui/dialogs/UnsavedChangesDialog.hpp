#pragma once

#include <functional>

namespace yqlora::editor {

class EditorSession;

// Вопрос "Сохранить изменения в сцене?" перед тем, как их можно потерять.
class UnsavedChangesDialog {
public:
    // modified - есть ли несохранённое, что важно для этого шага: при
    // смене сцены - только сцена (настройки проекта остаются), при закрытии
    // проекта - всё (EditorSession::modified). Если нет, then выполняется
    // сразу, иначе после ответа "Сохранить" (сохраняется весь проект) или
    // "Не сохранять". По "Отмене" не выполняется.
    void request(bool modified, std::function<void()> then);
    void draw(EditorSession& session);

private:
    std::function<void()> m_then;
    bool m_openRequested = false;
};

} // namespace yqlora::editor
