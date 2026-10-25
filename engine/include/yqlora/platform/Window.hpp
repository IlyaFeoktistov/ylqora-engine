#pragma once

#include "yqlora/platform/Key.hpp"
#include "yqlora/render/GraphicsApi.hpp"

#include <functional>
#include <string>

struct SDL_Window;

namespace yqlora {

struct WindowDesc {
    std::string title = "Yqlora";
    int width = 1280;
    int height = 720;
    bool resizable = true;
    // От API зависит, с каким флагом SDL создаёт окно (OpenGL/Vulkan/...).
    GraphicsApi api = GraphicsApi::OpenGL;
};

struct WindowEvents {
    std::function<void()> onCloseRequested;
    std::function<void(int width, int height)> onResized; // размер в пикселях
    std::function<void(Key key)> onKeyPressed;
};

class Window {
public:
    explicit Window(const WindowDesc& desc);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Обрабатывает все накопившиеся события ОС и вызывает нужные обработчики.
    void pollEvents(const WindowEvents& events);

    void setTitle(const std::string& title);

    int widthInPixels() const;
    int heightInPixels() const;

    // Для бэкендов рендеринга: им нужно окно SDL, чтобы создать контекст/поверхность.
    SDL_Window* sdlWindow() const { return m_window; }

private:
    SDL_Window* m_window = nullptr;
};

} // namespace yqlora
