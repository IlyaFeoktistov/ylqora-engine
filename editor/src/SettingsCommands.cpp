#include "yqlora/editor/SettingsCommands.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/project/ProjectSettings.hpp"

#include <utility>

namespace yqlora::editor {

namespace {

// Ставит значение; в файл раздел попадёт, когда проект сохранят. false,
// если раздела нет или значение не того типа.
bool setValue(EditorSession& session, const std::string& name, const Property& property, const PropertyValue& value)
{
    ProjectSettings* settings = session.project() ? &session.project()->settings() : nullptr;
    SettingsSection* section = settings ? settings->find(name) : nullptr;
    if (!section || !property.set(*section, value)) {
        return false;
    }
    settings->markChanged();

    return true;
}

} // namespace

SetSettingsPropertyCommand::SetSettingsPropertyCommand(
    std::string section,
    const Property& property,
    PropertyValue value
)
    : m_section(std::move(section))
    , m_property(property)
    , m_value(std::move(value))
{
}

bool SetSettingsPropertyCommand::apply(EditorSession& session)
{
    const SettingsSection* section = session.project() ? session.project()->settings().find(m_section) : nullptr;
    if (!section) {
        return false;
    }
    if (!m_previous) {
        m_previous = m_property.get(*section);
    }

    return setValue(session, m_section, m_property, m_value);
}

void SetSettingsPropertyCommand::revert(EditorSession& session)
{
    if (m_previous) {
        setValue(session, m_section, m_property, *m_previous);
    }
}

bool SetSettingsPropertyCommand::merge(const Command& next)
{
    const auto* other = dynamic_cast<const SetSettingsPropertyCommand*>(&next);
    if (!other || other->m_section != m_section || &other->m_property != &m_property) {
        return false;
    }
    m_value = other->m_value;

    return true;
}

} // namespace yqlora::editor
