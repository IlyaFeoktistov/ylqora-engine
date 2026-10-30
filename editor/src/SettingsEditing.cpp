#include "yqlora/editor/SettingsEditing.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/editor/SettingsCommands.hpp"
#include "yqlora/project/GeneralSettings.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

#include <algorithm>
#include <string>

namespace yqlora::editor {

bool SettingsEditing::setProperty(SettingsSection& section, const Property& property, const PropertyValue& value)
{
    return m_session.execute(
        std::make_unique<SetSettingsPropertyCommand>(std::string(section.typeName()), property, value)
    );
}

bool SettingsEditing::setStartupScene(const std::filesystem::path& path)
{
    auto& project = m_session.project();
    const ResourceId id = project ? project->resources().id(path) : 0;
    const TypeInfo* type = m_session.registry().find<GeneralSettings>();
    if (id == 0 || !type) {
        return false;
    }
    const auto properties = m_session.registry().properties(*type);
    const auto property = std::ranges::find(properties, kStartupSceneProperty, &Property::name);
    if (property == properties.end()) {
        return false;
    }

    m_session.finishEdit();
    const bool done = setProperty(
        project->settings().get<GeneralSettings>(),
        **property,
        PropertyValue{ResourceRef{id}}
    );
    m_session.finishEdit();

    return done;
}

} // namespace yqlora::editor
