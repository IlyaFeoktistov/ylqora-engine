#pragma once

#include <functional>

namespace yqlora::editor {

class ActionRegistry;
class EditorSession;

// Что делают действия, которые знает только приложение: сохранение с записью
// в лог, закрытие проекта и выход с вопросом о несохранённых изменениях.
struct EditorActionHandlers {
    std::function<void()> saveScene;
    std::function<void()> closeProject;
    std::function<void()> quit;
};

// Все действия редактора (см. ActionIds.hpp): подписи, клавиши и доступность.
void registerEditorActions(ActionRegistry& registry, EditorSession& session, EditorActionHandlers handlers);

} // namespace yqlora::editor
