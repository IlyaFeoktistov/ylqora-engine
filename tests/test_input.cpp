#include <doctest/doctest.h>

#include <yqlora/input/InputControl.hpp>
#include <yqlora/input/InputSettings.hpp>
#include <yqlora/platform/InputFeeder.hpp>
#include <yqlora/platform/MouseWrap.hpp>

#include <algorithm>

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

TEST_CASE("InputControl: keys and mouse buttons by name")
{
    CHECK(controlFromName("Space") == InputControl{Key::Space});
    CHECK(controlFromName("MouseRight") == InputControl{MouseButton::Right});
    CHECK_FALSE(controlFromName("Unknown"));
    CHECK_FALSE(controlFromName("NoSuchKey"));
    CHECK(controlName(InputControl{Key::F12}) == "F12");
    CHECK(controlName(InputControl{MouseButton::Left}) == "MouseLeft");
}

TEST_CASE("InputFeeder: actions from the settings, pressed and released by their own state")
{
    InputSettings settings;
    settings.actions = {
        {.name = "Jump", .kind = InputActionKind::Button, .bindings = {{.control = "Space"}, {.control = "MouseLeft"}}},
        {.name = "MoveX",
         .kind = InputActionKind::Axis,
         .bindings =
             {{.control = "D", .scale = 1.0f}, {.control = "A", .scale = -1.0f}, {.control = "Right", .scale = 1.0f}}},
    };
    Input input;
    InputFeeder feeder(input);

    feeder.keyDown(Key::Space);
    feeder.keyDown(Key::D);
    feeder.keyDown(Key::Right);
    feeder.updateActions(settings);
    CHECK(input.isActionDown("Jump"));
    CHECK(input.wasActionPressed("Jump"));
    CHECK(input.actionValue("Jump") == 1.0f);
    // Две привязки в одну сторону: ось не больше 1.
    CHECK(input.actionValue("MoveX") == 1.0f);

    // Вторая привязка держит действие: отпускания нет.
    feeder.beginFrame();
    feeder.mouseButtonDown(MouseButton::Left);
    feeder.keyUp(Key::Space);
    feeder.keyDown(Key::A);
    feeder.updateActions(settings);
    CHECK(input.isActionDown("Jump"));
    CHECK_FALSE(input.wasActionPressed("Jump"));
    CHECK_FALSE(input.wasActionReleased("Jump"));
    CHECK(input.actionValue("MoveX") == doctest::Approx(1.0f));

    feeder.beginFrame();
    feeder.mouseButtonUp(MouseButton::Left);
    feeder.keyUp(Key::D);
    feeder.keyUp(Key::Right);
    feeder.updateActions(settings);
    CHECK(input.wasActionReleased("Jump"));
    CHECK(input.actionValue("MoveX") == -1.0f);

    CHECK_FALSE(input.isActionDown("NoSuchAction"));
    CHECK(input.actionValue("NoSuchAction") == 0.0f);
}

TEST_CASE("InputFeeder: gamepad axes with a dead zone, released on disconnect")
{
    Input input;
    InputFeeder feeder(input);
    feeder.gamepadConnected(true);
    CHECK(input.gamepadConnected());

    // Стик в покое чуть отклонён: это ноль.
    feeder.gamepadAxisMoved(GamepadAxis::LeftStickX, 0.1f);
    CHECK(input.gamepadAxis(GamepadAxis::LeftStickX) == 0.0f);
    // За мёртвой зоной значение идёт от 0, без скачка, и доходит до 1.
    feeder.gamepadAxisMoved(GamepadAxis::LeftStickX, 0.21f);
    CHECK(input.gamepadAxis(GamepadAxis::LeftStickX) == doctest::Approx(0.0125f));
    feeder.gamepadAxisMoved(GamepadAxis::LeftStickY, -1.0f);
    CHECK(input.gamepadAxis(GamepadAxis::LeftStickY) == -1.0f);

    feeder.gamepadButtonDown(GamepadButton::South);
    CHECK(input.wasGamepadButtonPressed(GamepadButton::South));
    feeder.gamepadConnected(false);
    CHECK_FALSE(input.isGamepadButtonDown(GamepadButton::South));
    CHECK(input.wasGamepadButtonReleased(GamepadButton::South));
    CHECK(input.gamepadAxis(GamepadAxis::LeftStickY) == 0.0f);
}

TEST_CASE("InputControl: gamepad and mouse axes by name, the mouse is not limited")
{
    CHECK(controlFromName("GamepadSouth") == InputControl{GamepadButton::South});
    CHECK(controlFromName("GamepadLeftStickX") == InputControl{GamepadAxis::LeftStickX});
    CHECK(controlFromName("MouseX") == InputControl{MouseAxis::X});
    CHECK(controlName(InputControl{GamepadAxis::RightTrigger}) == "GamepadRightTrigger");
    // Escape есть, Unknown - нет.
    const auto all = allInputControls();
    CHECK(std::ranges::find(all, InputControl{Key::Escape}) != all.end());
    CHECK(std::ranges::find(all, InputControl{Key::Unknown}) == all.end());

    InputSettings settings;
    settings.actions = {
        {.name = "LookX",
         .kind = InputActionKind::Axis,
         .bindings = {{.control = "MouseX", .scale = 0.5f}, {.control = "GamepadRightStickX"}}},
        {.name = "Fire", .kind = InputActionKind::Button, .bindings = {{.control = "GamepadRightTrigger"}}},
    };
    Input input;
    InputFeeder feeder(input);
    feeder.gamepadConnected(true);
    feeder.mouseMoved({100.0f, 0.0f}, {30.0f, 0.0f});
    feeder.gamepadAxisMoved(GamepadAxis::RightTrigger, 0.8f);
    feeder.updateActions(settings);
    // Сдвиг мыши с чувствительностью 0.5 - 15, без ограничения 1.
    CHECK(input.actionValue("LookX") == 15.0f);
    CHECK(input.isActionDown("Fire"));
}

TEST_CASE("InputSettings: a new project starts with the default actions for keyboard, mouse and gamepad")
{
    const InputSettings settings;
    CHECK(settings.actions.size() == defaultInputActions().size());
    // Все привязки стандартного набора - настоящие органы управления.
    for (const InputAction& action : settings.actions) {
        CAPTURE(action.name);
        CHECK_FALSE(action.bindings.empty());
        for (const InputBinding& binding : action.bindings) {
            CAPTURE(binding.control);
            CHECK(controlFromName(binding.control));
        }
    }

    Input input;
    InputFeeder feeder(input);
    feeder.gamepadConnected(true);
    feeder.keyDown(Key::W);
    feeder.gamepadButtonDown(GamepadButton::South);
    feeder.updateActions(settings);
    CHECK(input.actionValue("MoveY") == 1.0f);
    CHECK(input.wasActionPressed("Jump"));
    CHECK_FALSE(input.isActionDown("Fire"));
}
