#include "yqlora/scene/TimeSettings.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/project/Project.hpp"

#include <algorithm>

namespace yqlora {

void TimeSettings::setFixedStep(float seconds)
{
    m_fixedStep = std::clamp(seconds, kMinTimeStep, kMaxTimeStep);
}

void TimeSettings::setMaxFrame(float seconds)
{
    m_maxFrame = std::clamp(seconds, kMinTimeStep, kMaxTimeStep);
}

const TimeSettings& timeSettingsOf(const EngineServices& services)
{
    static const TimeSettings defaults;
    const Project* project = services.project();
    const SettingsSection* section = project ? project->settings().find(typeNameOf<TimeSettings>()) : nullptr;

    return section ? static_cast<const TimeSettings&>(*section) : defaults;
}

} // namespace yqlora
