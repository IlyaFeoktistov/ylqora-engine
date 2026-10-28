#include <doctest/doctest.h>

#include <yqlora/core/FrameClock.hpp>

using namespace yqlora;

TEST_CASE("FrameClock: time between ticks, a long frame is cut")
{
    FrameClock clock(0.1);
    clock.reset(10.0);

    CHECK(clock.tick(10.016) == doctest::Approx(0.016f));
    CHECK(clock.tick(10.032) == doctest::Approx(0.016f));

    // Пауза в отладчике на 5 секунд - шаг не длиннее предела.
    CHECK(clock.tick(15.0) == doctest::Approx(0.1f));
    CHECK(clock.tick(15.01) == doctest::Approx(0.01f));

    // Часы назад не идут.
    CHECK(clock.tick(14.0) == 0.0f);
}
