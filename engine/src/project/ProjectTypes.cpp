#include "yqlora/project/ProjectTypes.hpp"

#include "yqlora/project/GeneralSettings.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

namespace yqlora {

void registerProjectTypes(TypeRegistry& registry)
{
    registry.registerSettings<SettingsSection>();

    registry.registerSettings<GeneralSettings>("general")
        .property("gameName", &GeneralSettings::gameName)
        .property("gameVersion", &GeneralSettings::gameVersion)
        .property("company", &GeneralSettings::company)
        .property(kStartupSceneProperty, &GeneralSettings::startupScene, PropertyHint::Scene)
        .property("icon", &GeneralSettings::icon, PropertyHint::Texture);
}

} // namespace yqlora
