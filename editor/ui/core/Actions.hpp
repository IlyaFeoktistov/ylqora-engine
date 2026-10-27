#pragma once

#include <imgui.h>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

// Команда редактора, которую вызывают из меню, кнопкой или горячей клавишей.
// Подпись, клавиши и доступность заданы в одном месте, поэтому меню
// и клавиши не расходятся. Настраивается цепочкой после ActionRegistry::add.
class Action {
public:
    Action(std::string id, std::string labelKey);

    Action& shortcut(ImGuiKeyChord chord);
    Action& alternativeShortcut(ImGuiKeyChord chord);
    // Без условия действие доступно всегда.
    Action& enabledWhen(std::function<bool()> condition);
    Action& onRun(std::function<void()> run);

    const std::string& id() const
    {
        return m_id;
    }

    const std::string& labelKey() const
    {
        return m_labelKey;
    }

    // 0 - без горячей клавиши.
    ImGuiKeyChord shortcutChord() const
    {
        return m_shortcut;
    }

    bool enabled() const;
    // Ничего не делает, если действие недоступно.
    void run() const;

    // Нажата одна из клавиш действия.
    bool shortcutPressed() const;

private:
    std::string m_id;
    std::string m_labelKey;
    ImGuiKeyChord m_shortcut = 0;
    ImGuiKeyChord m_alternativeShortcut = 0;
    std::function<bool()> m_enabled;
    std::function<void()> m_run;
};

class ActionRegistry {
public:
    Action& add(std::string id, std::string labelKey);

    const Action* find(std::string_view id) const;
    bool enabled(std::string_view id) const;
    void run(std::string_view id) const;

    // Горячие клавиши всех действий, вызывать раз за кадр.
    void handleShortcuts() const;

    // Пункт меню с подписью на языке интерфейса и клавишами.
    void menuItem(std::string_view id) const;

    // "Ctrl+S" или пустая строка, если клавиш нет.
    std::string shortcutText(std::string_view id) const;

private:
    // Указатели, чтобы ссылка из add не терялась при росте списка.
    std::vector<std::unique_ptr<Action>> m_actions;
};

} // namespace yqlora
