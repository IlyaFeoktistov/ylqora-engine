#include "yqlora/platform/Gamepads.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/platform/InputFeeder.hpp"

#include <SDL3/SDL.h>

#include <algorithm>
#include <optional>

namespace yqlora {

namespace {

// Полный ход оси в SDL.
constexpr float kAxisRange = 32767.0f;

std::optional<GamepadButton> toButton(Uint8 button)
{
    switch (button) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return GamepadButton::South;
    case SDL_GAMEPAD_BUTTON_EAST: return GamepadButton::East;
    case SDL_GAMEPAD_BUTTON_WEST: return GamepadButton::West;
    case SDL_GAMEPAD_BUTTON_NORTH: return GamepadButton::North;
    case SDL_GAMEPAD_BUTTON_BACK: return GamepadButton::Back;
    case SDL_GAMEPAD_BUTTON_GUIDE: return GamepadButton::Guide;
    case SDL_GAMEPAD_BUTTON_START: return GamepadButton::Start;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK: return GamepadButton::LeftStick;
    case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return GamepadButton::RightStick;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return GamepadButton::LeftShoulder;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return GamepadButton::RightShoulder;
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return GamepadButton::DpadUp;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return GamepadButton::DpadDown;
    case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return GamepadButton::DpadLeft;
    case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return GamepadButton::DpadRight;
    default: return std::nullopt;
    }
}

std::optional<GamepadAxis> toAxis(Uint8 axis)
{
    switch (axis) {
    case SDL_GAMEPAD_AXIS_LEFTX: return GamepadAxis::LeftStickX;
    case SDL_GAMEPAD_AXIS_LEFTY: return GamepadAxis::LeftStickY;
    case SDL_GAMEPAD_AXIS_RIGHTX: return GamepadAxis::RightStickX;
    case SDL_GAMEPAD_AXIS_RIGHTY: return GamepadAxis::RightStickY;
    case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: return GamepadAxis::LeftTrigger;
    case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: return GamepadAxis::RightTrigger;
    default: return std::nullopt;
    }
}

// SDL считает ось Y стика вниз, движок - вверх, как клавишу W.
float axisValue(GamepadAxis axis, Sint16 raw)
{
    const float value = std::clamp(static_cast<float>(raw) / kAxisRange, -1.0f, 1.0f);
    const bool vertical = axis == GamepadAxis::LeftStickY || axis == GamepadAxis::RightStickY;

    return vertical ? -value : value;
}

} // namespace

Gamepads::Gamepads()
{
    m_initialized = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (!m_initialized) {
        log::warn("Gamepads are not available: {}", SDL_GetError());
    }
}

Gamepads::~Gamepads()
{
    for (const std::uint32_t id : m_open) {
        SDL_CloseGamepad(SDL_GetGamepadFromID(id));
    }
    if (m_initialized) {
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }
}

bool Gamepads::handleEvent(const SDL_Event& event, InputFeeder& input)
{
    switch (event.type) {
    case SDL_EVENT_GAMEPAD_ADDED: connected(event.gdevice.which, input); return true;
    case SDL_EVENT_GAMEPAD_REMOVED: disconnected(event.gdevice.which, input); return true;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP: {
        const auto button = toButton(event.gbutton.button);
        if (event.gbutton.which == m_active && button) {
            if (event.gbutton.down) {
                input.gamepadButtonDown(*button);
            } else {
                input.gamepadButtonUp(*button);
            }
        }

        return true;
    }
    case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
        const auto axis = toAxis(event.gaxis.axis);
        if (event.gaxis.which == m_active && axis) {
            input.gamepadAxisMoved(*axis, axisValue(*axis, event.gaxis.value));
        }

        return true;
    }
    default: return false;
    }
}

void Gamepads::connected(std::uint32_t id, InputFeeder& input)
{
    SDL_Gamepad* gamepad = SDL_OpenGamepad(id);
    if (!gamepad) {
        log::warn("Gamepad cannot be opened: {}", SDL_GetError());

        return;
    }
    log::info("Gamepad connected: {}", SDL_GetGamepadName(gamepad));
    m_open.push_back(id);
    if (m_active == 0) {
        m_active = id;
        input.gamepadConnected(true);
    }
}

void Gamepads::disconnected(std::uint32_t id, InputFeeder& input)
{
    if (SDL_Gamepad* gamepad = SDL_GetGamepadFromID(id)) {
        SDL_CloseGamepad(gamepad);
    }
    std::erase(m_open, id);
    if (id != m_active) {
        return;
    }
    input.gamepadConnected(false);
    m_active = m_open.empty() ? 0 : m_open.front();
    if (m_active != 0) {
        input.gamepadConnected(true);
    }
}

} // namespace yqlora
