#include "yqlora/platform/Window.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace yqlora {

namespace {

SDL_WindowFlags apiFlag(GraphicsApi api)
{
    switch (api) {
    case GraphicsApi::OpenGL: return SDL_WINDOW_OPENGL;
    case GraphicsApi::Vulkan: return SDL_WINDOW_VULKAN;
    case GraphicsApi::Direct3D12: return 0;
    }

    return 0;
}

Key toKey(SDL_Scancode scancode)
{
    switch (scancode) {
    case SDL_SCANCODE_ESCAPE: return Key::Escape;
    case SDL_SCANCODE_F1: return Key::F1;
    case SDL_SCANCODE_F2: return Key::F2;
    default: return Key::Unknown;
    }
}

} // namespace

Window::Window(const WindowDesc& desc)
{
    SDL_WindowFlags flags = apiFlag(desc.api) | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (desc.resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    m_window = SDL_CreateWindow(desc.title.c_str(), desc.width, desc.height, flags);
    if (!m_window) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
}

Window::~Window()
{
    SDL_DestroyWindow(m_window);
}

void Window::pollEvents(const WindowEvents& events)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (events.onCloseRequested) {
                events.onCloseRequested();
            }
            break;
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            if (events.onResized) {
                events.onResized(event.window.data1, event.window.data2);
            }
            break;
        case SDL_EVENT_KEY_DOWN:
            if (!event.key.repeat && events.onKeyPressed) {
                events.onKeyPressed(toKey(event.key.scancode));
            }
            break;
        default:
            break;
        }
    }
}

void Window::setTitle(const std::string& title)
{
    SDL_SetWindowTitle(m_window, title.c_str());
}

int Window::widthInPixels() const
{
    int w = 0;
    SDL_GetWindowSizeInPixels(m_window, &w, nullptr);

    return w;
}

int Window::heightInPixels() const
{
    int h = 0;
    SDL_GetWindowSizeInPixels(m_window, nullptr, &h);

    return h;
}

} // namespace yqlora
