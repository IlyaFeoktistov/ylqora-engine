#pragma once

#include <memory>
#include <span>
#include <string>
#include <utility>

namespace yqlora {

class Application;

} // namespace yqlora

namespace yqlora::editor {

class ActionRegistry;
class EditorPanel;
class EditorSession;
class Theme;

struct MainMenuContext {
    Application& app;
    EditorSession& session;
    const ActionRegistry& actions;
    std::span<const std::unique_ptr<EditorPanel>> panels;
    // Код языка и его название.
    std::span<const std::pair<std::string, std::string>> languages;
    const Theme& theme;
};

// Главное меню окна: Файл, Правка, Вид с выбором языка, а по центру при
// открытом проекте - Play, Pause и Stop.
void drawMainMenu(const MainMenuContext& context);

} // namespace yqlora::editor
