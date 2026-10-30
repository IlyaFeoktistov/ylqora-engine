#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/scene/GameTime.hpp"

namespace yqlora {

class EngineServices;

// Пределы обоих времён: меньше миллисекунды шаг бессмыслен, больше секунды -
// игра на глазах дёргается.
inline constexpr float kMinTimeStep = 0.001f;
inline constexpr float kMaxTimeStep = 1.0f;
// Самый длинный кадр по умолчанию: после паузы в отладчике игра не прыгает на секунды вперёд.
inline constexpr float kDefaultMaxFrame = 0.1f;

// Раздел Time настроек проекта. Применяется сразу, в том числе во время Play:
// сцена и цикл кадра читают его каждый кадр.
class YQLORA_API TimeSettings final : public SettingsSection {
public:
    YQLORA_TYPE(TimeSettings, SettingsSection)

    // Шаг onFixedUpdate и физики, секунды.
    float fixedStep() const
    {
        return m_fixedStep;
    }

    // Самый длинный кадр, секунды: длиннее кадр укорачивается.
    float maxFrame() const
    {
        return m_maxFrame;
    }

    // Значения вне [kMinTimeStep, kMaxTimeStep] прижимаются к пределам.
    void setFixedStep(float seconds);
    void setMaxFrame(float seconds);

private:
    float m_fixedStep = kDefaultFixedStep;
    float m_maxFrame = kDefaultMaxFrame;
};

// Раздел Time открытого проекта, а без проекта - значения по умолчанию.
YQLORA_API const TimeSettings& timeSettingsOf(const EngineServices& services);

} // namespace yqlora
