#include "yqlora/platform/MouseWrap.hpp"

#include <cmath>

namespace yqlora {

namespace {

// Отступ от края: у самой границы окна события мыши могут уже не приходить.
constexpr float kMargin = 2.0f;
// Куда переносить курсор: ещё на пиксель внутрь, чтобы не сработать снова.
constexpr float kInsideMargin = 1.0f;
// Курсор в пределах этой доли окна от точки переноса: перенос сработал.
constexpr float kWarpTolerance = 0.25f;
// Скачок больше этой доли окна - это перенос, а не движение мыши.
constexpr float kJumpShare = 0.5f;

} // namespace

void MouseWrap::reset(const Vec2& position)
{
    m_lastPosition = position;
    m_warpTarget.reset();
    m_missedWarps = 0;
}

Vec2 MouseWrap::delta(const Vec2& position, const Vec2& windowSize)
{
    // Курсор недалеко от точки переноса: перенос сработал.
    if (m_warpTarget) {
        const Vec2 offset = position - *m_warpTarget;
        if (std::abs(offset.x) <= windowSize.x * kWarpTolerance
            && std::abs(offset.y) <= windowSize.y * kWarpTolerance) {
            m_warpTarget.reset();
            m_missedWarps = 0;
        } else {
            ++m_missedWarps;
        }
    }

    const Vec2 delta = position - m_lastPosition;
    m_lastPosition = position;
    if (std::abs(delta.x) > windowSize.x * kJumpShare || std::abs(delta.y) > windowSize.y * kJumpShare) {
        return Vec2(0.0f);
    }

    return delta;
}

std::optional<Vec2> MouseWrap::wrapTarget(const Vec2& position, const Vec2& windowSize) const
{
    Vec2 target = position;
    if (position.x <= kMargin) {
        target.x = windowSize.x - kMargin - kInsideMargin;
    } else if (position.x >= windowSize.x - kMargin) {
        target.x = kMargin + kInsideMargin;
    }
    if (position.y <= kMargin) {
        target.y = windowSize.y - kMargin - kInsideMargin;
    } else if (position.y >= windowSize.y - kMargin) {
        target.y = kMargin + kInsideMargin;
    }

    return target != position ? std::optional<Vec2>(target) : std::nullopt;
}

void MouseWrap::warped(const Vec2& target)
{
    m_lastPosition = target;
    m_warpTarget = target;
}

} // namespace yqlora
