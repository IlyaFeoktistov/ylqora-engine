#pragma once

#include "core/EditorPanel.hpp"

#include <string>

namespace yqlora::editor {

// Настройки проекта (Проект -> Настройки проекта): слева разделы, справа
// свойства выбранного. Правка идёт командами с отменой и сразу пишется в
// файл раздела. Открывается из меню, в начале скрыто.
class ProjectSettingsPanel final : public EditorPanel {
public:
    ProjectSettingsPanel()
    {
        setVisible(false);
    }

    const char* id() const override
    {
        return "ProjectSettings";
    }

    const char* titleKey() const override
    {
        return "editor.window.projectSettings";
    }

    // Показывает окно и выводит его вперёд.
    void open()
    {
        setVisible(true);
        m_focusRequested = true;
    }

    void draw(EditorContext& context) override;

private:
    // Имя типа выбранного раздела; пусто - первый.
    std::string m_selected;
    bool m_focusRequested = false;
};

} // namespace yqlora::editor
