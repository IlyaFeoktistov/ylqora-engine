#include "yqlora/render/GraphicsSettings.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/project/Project.hpp"

namespace yqlora {

GraphicsApi GraphicsSettings::currentPlatformApi() const
{
#ifdef _WIN32
    return windowsApi;
#else
    return linuxApi;
#endif
}

const GraphicsSettings& graphicsSettingsOf(const EngineServices& services)
{
    static const GraphicsSettings defaults;
    const Project* project = services.project();
    const SettingsSection* section = project ? project->settings().find(typeNameOf<GraphicsSettings>()) : nullptr;

    return section ? static_cast<const GraphicsSettings&>(*section) : defaults;
}

} // namespace yqlora
