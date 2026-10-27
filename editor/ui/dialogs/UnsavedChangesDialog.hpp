#pragma once

#include <functional>

namespace yqlora {

class EditorSession;

// Вопрос "Сохранить изменения в сцене?" перед тем, как их можно потерять.
class UnsavedChangesDialog {
public:
    // Если изменений нет, then выполняется сразу, иначе после ответа
    // "Сохранить" или "Не сохранять". По "Отмене" не выполняется.
    void request(const EditorSession& session, std::function<void()> then);
    void draw(EditorSession& session);

private:
    std::function<void()> m_then;
    bool m_openRequested = false;
};

} // namespace yqlora
