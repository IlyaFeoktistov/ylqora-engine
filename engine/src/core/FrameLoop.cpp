#include "yqlora/core/FrameLoop.hpp"

namespace yqlora {

FrameLoop::FrameLoop(FrameStages& stages, double maxFrameSeconds)
    : m_stages(stages)
    , m_clock(maxFrameSeconds)
{
}

void FrameLoop::reset(double nowSeconds)
{
    m_clock.reset(nowSeconds);
}

bool FrameLoop::runFrame(double nowSeconds)
{
    const float deltaSeconds = m_clock.tick(nowSeconds);
    if (!m_stages.input()) {
        return false;
    }
    m_stages.simulate(deltaSeconds);
    m_stages.updateTransforms();
    m_stages.extract();
    m_stages.render();
    m_stages.interface();
    m_stages.present();

    return true;
}

} // namespace yqlora
