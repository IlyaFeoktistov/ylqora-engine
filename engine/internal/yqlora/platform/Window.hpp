#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/platform/MouseWrap.hpp"
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

    // Пока включено, курсор, дошедший до края окна, переносится к противоположному
    // краю, а смещение мыши считается без скачка. Так обзор камерой в редакторе
    // не упирается в край экрана, а курсор остаётся видно.
    //
    // Если система молча не переносит курсор (так в WSLg), перенос выключается
    // до конца работы: обзор работает, но у края экрана поворот останавливается.
    void setMouseWrapping(bool wrapping);

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
    // Смещение мыши в режиме переноса; переносит курсор, если он у края.
    Vec2 wrappedMouseDelta(const Vec2& position);

    SDL_Window* m_window = nullptr;
    MouseWrap m_mouseWrap;
    bool m_mouseWrapping = false;
    // Выяснилось, что система не переносит курсор: больше не пытаемся.
    bool m_warpUnsupported = false;
};

} // namespace yqlora
