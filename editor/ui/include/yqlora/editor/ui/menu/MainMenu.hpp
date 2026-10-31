#pragma once

#include <functional>
#include <memory>
#include <span>
#include <string>
#include <utility>

namespace yqlora::editor {

class ActionRegistry;
class EditorHost;
class EditorPanel;
class EditorSession;
class LogBuffer;
class Theme;

struct MainMenuContext {
    const EditorHost& host;
    EditorSession& session;
    const ActionRegistry& actions;
    std::span<const std::unique_ptr<EditorPanel>> panels;
    // Код языка и его название.
    std::span<const std::pair<std::string, std::string>> languages;
    const Theme& theme;
    const LogBuffer& log;
    // Открыть панель "Лог": по щелчку на счётчике ошибок.
    std::function<void()> showLog;
};

// Главное меню окна: Файл, Правка, Вид с выбором языка, а при открытом
// проекте по центру Play, Pause и Stop, справа счётчики ошибок и
// предупреждений лога.
void drawMainMenu(const MainMenuContext& context);

} // namespace yqlora::editor
