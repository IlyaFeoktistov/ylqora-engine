#include <doctest/doctest.h>

#include <yqlora/core/FrameClock.hpp>
#include <yqlora/core/FrameLoop.hpp>

#include <string>
#include <vector>

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

namespace {

// Записывает стадии кадра в том порядке, в каком их вызвали.
struct RecordingStages final : FrameStages {
    bool input() override
    {
        calls.emplace_back("input");

        return !closing;
    }

    void simulate(float deltaSeconds) override
    {
        calls.emplace_back("simulate");
        lastDelta = deltaSeconds;
    }

    void updateTransforms() override
    {
        calls.emplace_back("transforms");
    }

    void extract() override
    {
        calls.emplace_back("extract");
    }

    void render() override
    {
        calls.emplace_back("render");
    }

    void interface() override
    {
        calls.emplace_back("interface");
    }

    void present() override
    {
        calls.emplace_back("present");
    }

    std::vector<std::string> calls;
    float lastDelta = 0.0f;
    bool closing = false;
};

} // namespace

TEST_CASE("FrameLoop: stages in the order of the frame pipeline")
{
    RecordingStages stages;
    FrameLoop loop(stages, 0.1);
    loop.reset(1.0);

    CHECK(loop.runFrame(1.02));
    CHECK(
        stages.calls
        == std::vector<std::string>{"input", "simulate", "transforms", "extract", "render", "interface", "present"}
    );
    CHECK(stages.lastDelta == doctest::Approx(0.02f));

    // Окно закрывают: после ввода кадр дальше не идёт.
    stages.calls.clear();
    stages.closing = true;
    CHECK_FALSE(loop.runFrame(1.04));
    CHECK(stages.calls == std::vector<std::string>{"input"});
}
