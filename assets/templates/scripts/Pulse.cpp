#include "Pulse.hpp"

#include <yqlora/scene/Transform3D.hpp>

#include <glm/gtc/constants.hpp>

#include <cmath>

void Pulse::onStart()
{
    m_baseScale = transform().scale();
}

void Pulse::onUpdate(float deltaSeconds)
{
    m_elapsed += deltaSeconds;
    const float wave = std::sin(m_elapsed * frequency * glm::two_pi<float>());
    transform().setScale(m_baseScale * (1.0f + amplitude * wave));
}
