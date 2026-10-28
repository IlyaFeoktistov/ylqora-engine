#pragma once

#include "yqlora/core/Export.hpp"

namespace yqlora {

// Длина кадра по часам. Слишком длинный кадр (паузу в отладчике, перетаскивание
// окна) укорачивает до maxFrameSeconds, чтобы игра не прыгала на секунды вперёд.
class YQLORA_API FrameClock {
public:
    explicit FrameClock(double maxFrameSeconds);

    // Начинает отсчёт с момента nowSeconds.
    void reset(double nowSeconds);

    // Сколько прошло с прошлого вызова (или reset), не больше maxFrameSeconds.
    float tick(double nowSeconds);

private:
    double m_maxFrameSeconds;
    double m_previous = 0.0;
};

} // namespace yqlora
