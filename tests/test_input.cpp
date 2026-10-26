#include <doctest/doctest.h>

#include <yqlora/platform/InputFeeder.hpp>

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
