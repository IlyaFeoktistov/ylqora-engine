#pragma once

#include <imgui.h>

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

// Команда редактора, которую вызывают из меню, кнопкой или горячей клавишей.
// Подпись, клавиши и доступность заданы в одном месте, поэтому меню
// и клавиши не расходятся.
struct Action {
    std::string id;
    std::string labelKey;
    // 0 - без горячей клавиши.
    ImGuiKeyChord shortcut = 0;
    ImGuiKeyChord alternativeShortcut = 0;
    // Пусто - доступно всегда.
    std::function<bool()> enabled = nullptr;
    std::function<void()> run;
};

class ActionRegistry {
public:
    void add(Action action);

    const Action* find(std::string_view id) const;
    bool enabled(std::string_view id) const;
    void run(std::string_view id);

    // Горячие клавиши всех действий, вызывать раз за кадр.
    void handleShortcuts();

    // Пункт меню с подписью на языке интерфейса и клавишами.
    void menuItem(std::string_view id);

    // "Ctrl+S" или пустая строка, если клавиш нет.
    std::string shortcutText(std::string_view id) const;

private:
    std::vector<Action> m_actions;
};

} // namespace yqlora
