#pragma once

#include "yqlora/render/FrameData.hpp"
#include "yqlora/render/GraphicsApi.hpp"

#include <memory>

namespace yqlora {

class Window;

// Контракт движка на отрисовку. Реализации живут в backends/<api>/
// и не светят заголовки своего API наружу.
class Renderer {
public:
    virtual ~Renderer() = default;

    virtual GraphicsApi api() const = 0;
    virtual DepthRange depthRange() const = 0;

    // Рисует кадр в окно. На экране он появится после present(): между ними
    // поверх можно дорисовать интерфейс.
    virtual void render(const FrameData& frame) = 0;

    // Показывает нарисованный кадр на экране.
    virtual void present() = 0;

    // Размер области вывода в пикселях изменился.
    virtual void resize(int width, int height) = 0;
};

// Настраивает параметры, которые API требует задать до создания окна
// (например, версию контекста OpenGL). Вызывается после platform::initialize().
void prepareWindowForApi(GraphicsApi api);

// Создаёт рендерер для окна. Бросает std::runtime_error, если API
// не собран в этой сборке или не поддерживается системой.
std::unique_ptr<Renderer> createRenderer(GraphicsApi api, Window& window);

} // namespace yqlora
