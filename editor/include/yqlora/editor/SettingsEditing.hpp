#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/reflection/Property.hpp"

#include <filesystem>
#include <memory>

namespace yqlora {
class SettingsSection;
} // namespace yqlora

namespace yqlora::editor {

class EditorSession;

// Правка настроек открытого проекта командами с отменой, в той же истории,
// что и правка сцены. В файлы разделов значения попадают при сохранении
// проекта вместе со сценой.
class SettingsEditing {
public:
    explicit SettingsEditing(EditorSession& session)
        : m_session(session)
    {
    }

    bool setProperty(SettingsSection& section, const Property& property, const PropertyValue& value);

    // Делает сцену стартовой (GeneralSettings::startupScene): она откроется
    // вместе с проектом и запустится в игре.
    bool setStartupScene(const std::filesystem::path& path);

private:
    EditorSession& m_session;
};

} // namespace yqlora::editor
