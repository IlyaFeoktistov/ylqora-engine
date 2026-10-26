#include <doctest/doctest.h>

#include <yqlora/platform/InputFeeder.hpp>
#include <yqlora/platform/MouseWrap.hpp>

using namespace yqlora;

TEST_CASE("Input: pressed and released last one frame, down lasts while held")
{
    Input input;
    InputFeeder feeder(input);

    feeder.beginFrame();
    feeder.keyDown(Key::W);
    CHECK(input.isKeyDown(Key::W));
    CHECK(input.wasKeyPressed(Key::W));

    // Следующий кадр: клавиша всё ещё зажата, но нажатием это уже не считается.
    feeder.beginFrame();
    CHECK(input.isKeyDown(Key::W));
    CHECK_FALSE(input.wasKeyPressed(Key::W));

    feeder.keyUp(Key::W);
    CHECK_FALSE(input.isKeyDown(Key::W));
    CHECK(input.wasKeyReleased(Key::W));

    feeder.beginFrame();
    CHECK_FALSE(input.wasKeyReleased(Key::W));
}

TEST_CASE("Input: mouse movement and wheel add up within a frame")
{
    Input input;
    InputFeeder feeder(input);

    feeder.beginFrame();
    feeder.mouseMoved({10.0f, 20.0f}, {3.0f, -1.0f});
    feeder.mouseMoved({12.0f, 25.0f}, {2.0f, 5.0f});
    feeder.wheelScrolled(1.0f);
    feeder.wheelScrolled(1.0f);
    CHECK(input.mousePosition() == Vec2(12.0f, 25.0f));
    CHECK(input.mouseDelta() == Vec2(5.0f, 4.0f));
    CHECK(input.wheelDelta() == doctest::Approx(2.0f));

    // Положение курсора остаётся, смещение и колесо обнуляются.
    feeder.beginFrame();
    CHECK(input.mousePosition() == Vec2(12.0f, 25.0f));
    CHECK(input.mouseDelta() == Vec2(0.0f));
    CHECK(input.wheelDelta() == doctest::Approx(0.0f));
}

TEST_CASE("Input: losing focus releases everything")
{
    Input input;
    InputFeeder feeder(input);

    feeder.beginFrame();
    feeder.keyDown(Key::LeftShift);
    feeder.mouseButtonDown(MouseButton::Right);

    feeder.releaseAll();
    CHECK_FALSE(input.isKeyDown(Key::LeftShift));
    CHECK_FALSE(input.isMouseButtonDown(MouseButton::Right));
    CHECK(input.wasMouseButtonReleased(MouseButton::Right));
}

TEST_CASE("MouseWrap: cursor at an edge goes to the opposite one")
{
    const MouseWrap wrap;
    const Vec2 window(800.0f, 600.0f);

    CHECK_FALSE(wrap.wrapTarget({400.0f, 300.0f}, window));
    CHECK(wrap.wrapTarget({799.0f, 300.0f}, window) == Vec2(3.0f, 300.0f));
    CHECK(wrap.wrapTarget({0.0f, 300.0f}, window) == Vec2(797.0f, 300.0f));
    // Курсор успел выскочить за окно: переносим так же.
    CHECK(wrap.wrapTarget({400.0f, -50.0f}, window) == Vec2(400.0f, 597.0f));
}

TEST_CASE("MouseWrap: the jump of a wrapped cursor is not counted as movement")
{
    MouseWrap wrap;
    const Vec2 window(800.0f, 600.0f);
    wrap.reset({790.0f, 300.0f});

    CHECK(wrap.delta({798.0f, 300.0f}, window) == Vec2(8.0f, 0.0f));
    wrap.warped({3.0f, 300.0f});
    CHECK(wrap.delta({10.0f, 300.0f}, window) == Vec2(7.0f, 0.0f));

    // Событие из очереди со старым положением у правого края пришло уже после
    // переноса. Скачок через полокна - не движение, поворота быть не должно.
    CHECK(wrap.delta({798.0f, 300.0f}, window) == Vec2(0.0f));
}

TEST_CASE("MouseWrap: detects a system that silently ignores warping")
{
    MouseWrap wrap;
    const Vec2 window(800.0f, 600.0f);
    wrap.reset({790.0f, 300.0f});

    // Курсор переносили к левому краю, а он всё время остаётся у правого.
    for (int i = 0; i < 3; ++i) {
        wrap.delta({799.0f, 300.0f}, window);
        CHECK_FALSE(wrap.warpIgnored());
        wrap.warped({3.0f, 300.0f});
    }
    wrap.delta({799.0f, 300.0f}, window);
    wrap.delta({799.0f, 300.0f}, window);
    CHECK(wrap.warpIgnored());
}

TEST_CASE("MouseWrap: a working warp is not mistaken for an ignored one")
{
    MouseWrap wrap;
    const Vec2 window(800.0f, 600.0f);
    wrap.reset({790.0f, 300.0f});

    for (int i = 0; i < 10; ++i) {
        wrap.warped({3.0f, 300.0f});
        // Сначала старое событие из очереди, потом курсор уже у левого края.
        wrap.delta({799.0f, 300.0f}, window);
        wrap.delta({5.0f, 300.0f}, window);
    }
    CHECK_FALSE(wrap.warpIgnored());
}
