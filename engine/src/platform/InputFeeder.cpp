#include "yqlora/platform/InputFeeder.hpp"

#include "yqlora/input/InputControl.hpp"
#include "yqlora/input/InputSettings.hpp"

#include <algorithm>

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

} // namespace

void InputFeeder::beginFrame()
{
    m_input.m_keysPressed.fill(false);
    m_input.m_keysReleased.fill(false);
    m_input.m_buttonsPressed.fill(false);
    m_input.m_buttonsReleased.fill(false);
    m_input.m_mouseDelta = Vec2(0.0f);
    m_input.m_wheelDelta = 0.0f;
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

void InputFeeder::releaseAll()
{
    for (std::size_t i = 0; i < m_input.m_keysDown.size(); ++i) {
        keyUp(static_cast<Key>(i));
    }
    for (std::size_t i = 0; i < m_input.m_buttonsDown.size(); ++i) {
        mouseButtonUp(static_cast<MouseButton>(i));
    }
}

void InputFeeder::updateActions(const InputSettings& settings)
{
    auto previous = std::exchange(m_input.m_actions, {});
    for (const InputAction& action : settings.actions) {
        Input::ActionState state;
        float sum = 0.0f;
        for (const InputBinding& binding : action.bindings) {
            const auto control = controlFromName(binding.control);
            if (control && isControlDown(m_input, *control)) {
                state.down = true;
                sum += binding.scale;
            }
        }
        state.value = action.kind == InputActionKind::Axis ? std::clamp(sum, -1.0f, 1.0f) : (state.down ? 1.0f : 0.0f);
        const auto before = previous.find(action.name);
        const bool wasDown = before != previous.end() && before->second.down;
        state.pressed = state.down && !wasDown;
        state.released = !state.down && wasDown;
        // При повторе имени действует первое, как и в порядке выполнения.
        m_input.m_actions.emplace(action.name, state);
    }
}

} // namespace yqlora
