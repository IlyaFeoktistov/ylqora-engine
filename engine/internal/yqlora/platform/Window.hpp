#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/render/GraphicsApi.hpp"

#include <functional>
#include <string>

struct SDL_Window;

namespace yqlora {

class InputFeeder;

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
};

class Window {
public:
    explicit Window(const WindowDesc& desc);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Обрабатывает все накопившиеся события ОС: клавиатуру и мышь передаёт
    // во ввод, остальное в обработчики.
    void pollEvents(const WindowEvents& events, InputFeeder& input);

    // Захваченный курсор скрыт и не упирается в края окна, а мышь отдаёт
    // только смещение. Так работает обзор камерой в редакторе.
    void setMouseCaptured(bool captured);

    void setTitle(const std::string& title);

    // В тех же единицах, что положение мыши. На экранах с масштабом
    // меньше размера в пикселях.
    Vec2 sizeInPoints() const;

    int widthInPixels() const;
    int heightInPixels() const;

    // Для бэкендов рендеринга: им нужно окно SDL, чтобы создать контекст/поверхность.
    SDL_Window* sdlWindow() const
    {
        return m_window;
    }

private:
    SDL_Window* m_window = nullptr;
};

} // namespace yqlora
