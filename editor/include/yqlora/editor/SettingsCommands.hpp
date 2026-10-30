#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/reflection/Property.hpp"

#include <optional>
#include <string>

namespace yqlora::editor {

// Свойство раздела настроек проекта. Раздел задан именем типа: так команда
// не держит указателей на объекты проекта. Правка действует сразу, в файл
// раздела попадает при сохранении проекта (EditorSession::save); сцену она
// не трогает. Правки одного свойства сливаются, пока не вызван finishEdit.
class SetSettingsPropertyCommand final : public Command {
public:
    SetSettingsPropertyCommand(std::string section, const Property& property, PropertyValue value);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;
    bool merge(const Command& next) override;

    bool changesScene() const override
    {
        return false;
    }

private:
    std::string m_section;
    const Property& m_property;
    PropertyValue m_value;
    std::optional<PropertyValue> m_previous;
};

} // namespace yqlora::editor
