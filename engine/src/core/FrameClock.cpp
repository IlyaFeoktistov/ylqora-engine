#include "yqlora/core/FrameClock.hpp"

#include <algorithm>

namespace yqlora {

FrameClock::FrameClock(double maxFrameSeconds)
    : m_maxFrameSeconds(maxFrameSeconds)
{
}

void FrameClock::reset(double nowSeconds)
{
    m_previous = nowSeconds;
}

float FrameClock::tick(double nowSeconds)
{
    const double elapsed = std::clamp(nowSeconds - m_previous, 0.0, m_maxFrameSeconds);
    m_previous = nowSeconds;

    return static_cast<float>(elapsed);
}

} // namespace yqlora
