#pragma once

#include <yqlora/math/Math.hpp>
#include <yqlora/scripting/Script.hpp>

inline constexpr float kMaxPulseAmplitude = 1.0f;
inline constexpr float kMaxPulseFrequency = 10.0f;

// Узел увеличивается и уменьшается.
class Pulse : public yqlora::Script {
public:
    YQLORA_SCRIPT(Pulse)

    float amplitude = 0.2f;
    float frequency = 1.0f;

    YQLORA_FIELDS(
        field("amplitude", &Pulse::amplitude)
            .range(0.0f, kMaxPulseAmplitude)
            .tooltip("How much the node grows and shrinks"),
        field("frequency", &Pulse::frequency).range(0.0f, kMaxPulseFrequency).tooltip("Pulses per second")
    )

protected:
    void onStart() override;
    void onUpdate(float deltaSeconds) override;

private:
    // Не перечислены в YQLORA_FIELDS: живут только во время игры.
    yqlora::Vec3 m_baseScale{1.0f};
    float m_elapsed = 0.0f;
};
