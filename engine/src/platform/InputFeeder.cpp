#include "yqlora/platform/InputFeeder.hpp"

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

} // namespace yqlora
