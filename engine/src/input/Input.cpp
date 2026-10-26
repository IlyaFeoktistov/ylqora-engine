#include "yqlora/input/Input.hpp"

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

bool Input::isKeyDown(Key key) const
{
    return m_keysDown[index(key)];
}

bool Input::wasKeyPressed(Key key) const
{
    return m_keysPressed[index(key)];
}

bool Input::wasKeyReleased(Key key) const
{
    return m_keysReleased[index(key)];
}

bool Input::isMouseButtonDown(MouseButton button) const
{
    return m_buttonsDown[index(button)];
}

bool Input::wasMouseButtonPressed(MouseButton button) const
{
    return m_buttonsPressed[index(button)];
}

bool Input::wasMouseButtonReleased(MouseButton button) const
{
    return m_buttonsReleased[index(button)];
}

Vec2 Input::mousePosition() const
{
    return m_mousePosition;
}

Vec2 Input::mouseDelta() const
{
    return m_mouseDelta;
}

float Input::wheelDelta() const
{
    return m_wheelDelta;
}

} // namespace yqlora
