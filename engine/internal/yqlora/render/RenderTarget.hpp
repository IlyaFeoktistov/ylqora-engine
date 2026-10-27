#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"

#include <cstdint>

namespace yqlora {

// Текстура для показа в интерфейсе (ImGui::Image). uv0 и uv1 задают углы:
// у OpenGL картинка в текстуре перевёрнута по вертикали, у других API нет.
struct UiTexture {
    std::uint64_t id = 0;
    Vec2 uv0{0.0f, 0.0f};
    Vec2 uv1{1.0f, 1.0f};
};

// Текстура, в которую рисуется кадр вместо окна. Например, окно сцены в редакторе.
class YQLORA_API RenderTarget {
public:
    virtual ~RenderTarget() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;

    // Размер в пикселях. Если он не изменился, ничего не делает.
    virtual void resize(int width, int height) = 0;

    virtual UiTexture uiTexture() const = 0;
};

} // namespace yqlora
