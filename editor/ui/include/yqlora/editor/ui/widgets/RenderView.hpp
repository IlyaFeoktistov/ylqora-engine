#pragma once

#include "yqlora/render/RenderTarget.hpp"

#include <imgui.h>

#include <memory>

namespace yqlora {

class Renderer;
struct FrameData;

} // namespace yqlora

namespace yqlora::editor {

// Кадр сцены внутри окна интерфейса: текстура под размер области, в неё
// рисуется кадр, а интерфейс показывает её. Порядок за кадр: resize, image,
// render. Интерфейс рисуется позже всего, так что кадр успевает в текстуру.
class RenderView {
public:
    // Подгоняет текстуру под область size в точках. До image: при смене
    // размера текстура может пересоздаться. false, если область пустая.
    bool resize(Renderer& renderer, const ImVec2& size);

    // Показывает текстуру на месте курсора, как обычный элемент окна.
    void image(const ImVec2& size) const;

    // Показывает текстуру в прямоугольнике поверх окна, не сдвигая курсор.
    void image(ImDrawList& list, const ImVec2& min, const ImVec2& max) const;

    void render(Renderer& renderer, const FrameData& frame);

    // Рендерер пересоздают: текстура уходит сейчас, новая создастся при
    // следующем resize уже у нового рендерера.
    void release()
    {
        m_target.reset();
    }

private:
    std::unique_ptr<RenderTarget> m_target;
};

} // namespace yqlora::editor
