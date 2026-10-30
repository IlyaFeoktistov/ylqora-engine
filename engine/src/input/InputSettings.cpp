#include "yqlora/input/InputSettings.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/project/Project.hpp"

namespace yqlora {

const InputSettings& inputSettingsOf(const EngineServices& services)
{
    static const InputSettings empty;
    const Project* project = services.project();
    const SettingsSection* section = project ? project->settings().find(typeNameOf<InputSettings>()) : nullptr;

    return section ? static_cast<const InputSettings&>(*section) : empty;
}

} // namespace yqlora
