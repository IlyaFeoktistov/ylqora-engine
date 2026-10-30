#include "yqlora/platform/InputFeeder.hpp"

#include "yqlora/input/InputControl.hpp"
#include "yqlora/input/InputSettings.hpp"

#include <algorithm>
#include <cmath>

namespace yqlora {

namespace {

std::size_t index(Key key)
{
    return static_cast<std::size_t>(key);
}

std::size_t index(MouseButton button)
{
    return static_cast<std::size_t>(button);
}

std::size_t index(GamepadButton button)
{
    return static_cast<std::size_t>(button);
}

// Стик в покое не стоит ровно в нуле, курок слегка нажат от пружины.
constexpr float kStickDeadZone = 0.2f;
constexpr float kTriggerDeadZone = 0.1f;

bool isTrigger(GamepadAxis axis)
{
    return axis == GamepadAxis::LeftTrigger || axis == GamepadAxis::RightTrigger;
}

// Внутри мёртвой зоны 0, дальше - плавно от 0 до 1, без скачка на её краю.
float applyDeadZone(float value, float deadZone)
{
    const float magnitude = std::abs(value);
    if (magnitude <= deadZone) {
        return 0.0f;
    }

    return std::copysign(std::min((magnitude - deadZone) / (1.0f - deadZone), 1.0f), value);
}

} // namespace

void InputFeeder::beginFrame()
{
    m_input.m_keysPressed.fill(false);
    m_input.m_keysReleased.fill(false);
    m_input.m_buttonsPressed.fill(false);
    m_input.m_buttonsReleased.fill(false);
    m_input.m_mouseDelta = Vec2(0.0f);
    m_input.m_wheelDelta = 0.0f;
    m_input.m_gamepadPressed.fill(false);
    m_input.m_gamepadReleased.fill(false);
}

void InputFeeder::keyDown(Key key)
{
    if (key == Key::Unknown || m_input.m_keysDown[index(key)]) {
        return;
    }
    m_input.m_keysDown[index(key)] = true;
    m_input.m_keysPressed[index(key)] = true;
}

void InputFeeder::keyUp(Key key)
{
    if (key == Key::Unknown || !m_input.m_keysDown[index(key)]) {
        return;
    }
    m_input.m_keysDown[index(key)] = false;
    m_input.m_keysReleased[index(key)] = true;
}

void InputFeeder::mouseButtonDown(MouseButton button)
{
    if (m_input.m_buttonsDown[index(button)]) {
        return;
    }
    m_input.m_buttonsDown[index(button)] = true;
    m_input.m_buttonsPressed[index(button)] = true;
}

void InputFeeder::mouseButtonUp(MouseButton button)
{
    if (!m_input.m_buttonsDown[index(button)]) {
        return;
    }
    m_input.m_buttonsDown[index(button)] = false;
    m_input.m_buttonsReleased[index(button)] = true;
}

void InputFeeder::mouseMoved(const Vec2& position, const Vec2& delta)
{
    m_input.m_mousePosition = position;
    m_input.m_mouseDelta += delta;
}

void InputFeeder::wheelScrolled(float delta)
{
    m_input.m_wheelDelta += delta;
}

void InputFeeder::gamepadConnected(bool connected)
{
    if (!connected) {
        for (std::size_t i = 0; i < m_input.m_gamepadDown.size(); ++i) {
            gamepadButtonUp(static_cast<GamepadButton>(i));
        }
        m_input.m_gamepadAxes.fill(0.0f);
    }
    m_input.m_gamepadConnected = connected;
}

void InputFeeder::gamepadButtonDown(GamepadButton button)
{
    if (m_input.m_gamepadDown[index(button)]) {
        return;
    }
    m_input.m_gamepadDown[index(button)] = true;
    m_input.m_gamepadPressed[index(button)] = true;
}

void InputFeeder::gamepadButtonUp(GamepadButton button)
{
    if (!m_input.m_gamepadDown[index(button)]) {
        return;
    }
    m_input.m_gamepadDown[index(button)] = false;
    m_input.m_gamepadReleased[index(button)] = true;
}

void InputFeeder::gamepadAxisMoved(GamepadAxis axis, float value)
{
    const float deadZone = isTrigger(axis) ? kTriggerDeadZone : kStickDeadZone;
    m_input.m_gamepadAxes[static_cast<std::size_t>(axis)] = applyDeadZone(value, deadZone);
}

void InputFeeder::releaseAll()
{
    for (std::size_t i = 0; i < m_input.m_keysDown.size(); ++i) {
        keyUp(static_cast<Key>(i));
    }
    for (std::size_t i = 0; i < m_input.m_buttonsDown.size(); ++i) {
        mouseButtonUp(static_cast<MouseButton>(i));
    }
    for (std::size_t i = 0; i < m_input.m_gamepadDown.size(); ++i) {
        gamepadButtonUp(static_cast<GamepadButton>(i));
    }
    m_input.m_gamepadAxes.fill(0.0f);
}

void InputFeeder::updateActions(const InputSettings& settings)
{
    auto previous = std::exchange(m_input.m_actions, {});
    for (const InputAction& action : settings.actions) {
        Input::ActionState state;
        float sum = 0.0f;
        bool unbounded = false;
        for (const InputBinding& binding : action.bindings) {
            const auto control = controlFromName(binding.control);
            const float value = control ? controlValue(m_input, *control) : 0.0f;
            if (value != 0.0f) {
                state.down = true;
                sum += value * binding.scale;
                unbounded = unbounded || isUnbounded(*control);
            }
        }
        // Мышь даёт сдвиг, а не отклонение: обзор мышью не упирается в 1.
        const float axis = unbounded ? sum : std::clamp(sum, -1.0f, 1.0f);
        state.value = action.kind == InputActionKind::Axis ? axis : (state.down ? 1.0f : 0.0f);
        const auto before = previous.find(action.name);
        const bool wasDown = before != previous.end() && before->second.down;
        state.pressed = state.down && !wasDown;
        state.released = !state.down && wasDown;
        // При повторе имени действует первое, как и в порядке выполнения.
        m_input.m_actions.emplace(action.name, state);
    }
}

} // namespace yqlora
