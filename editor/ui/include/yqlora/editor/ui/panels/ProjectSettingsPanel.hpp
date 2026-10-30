#pragma once

#include "yqlora/editor/ui/core/EditorPanel.hpp"

#include "yqlora/reflection/Property.hpp"

#include <optional>
#include <string>
#include <string_view>

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

    // Показывает окно и выводит его вперёд; section - имя типа раздела,
    // который выбрать, пусто - оставить выбранный.
    void open(std::string_view section = {})
    {
        setVisible(true);
        m_focusRequested = true;
        if (!section.empty()) {
            m_selected = section;
        }
    }

    void draw(EditorContext& context) override;

private:
    // Правка, которая ждёт ответа: она стирает имя слоя, на котором есть узлы.
    struct PendingEdit {
        std::string section;
        const Property* property = nullptr;
        PropertyValue value;
        std::string question;
    };

    // Ставит правку в очередь на вопрос, если она стирает имя занятого слоя.
    // true - правка отложена до ответа.
    bool askBeforeClearingLayers(
        EditorSession& session,
        const SettingsSection& section,
        const Property& property,
        const PropertyValue& value
    );
    void drawPendingQuestion(EditorSession& session);

    std::optional<PendingEdit> m_pending;
    // Имя типа выбранного раздела; пусто - первый.
    std::string m_selected;
    bool m_focusRequested = false;
};

} // namespace yqlora::editor
