#pragma once

#include "yqlora/render/Renderer.hpp"

#include <memory>

// Намеренно без glad/SDL: этот заголовок включает фабрика рендереров.
struct SDL_GLContextState;

namespace yqlora {

class GridPass;

class OpenGLRenderer final : public Renderer {
public:
    static constexpr int kVersionMajor = 3;
    static constexpr int kVersionMinor = 3;

    // Атрибуты контекста (версия, core profile, буферы), задаются до создания окна.
    static void prepareWindow();

    explicit OpenGLRenderer(Window& window);
    ~OpenGLRenderer() override;

    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer&) = delete;

    GraphicsApi api() const override
    {
        return GraphicsApi::OpenGL;
    }

    DepthRange depthRange() const override
    {
        return DepthRange::MinusOneToOne;
    }

    void render(const FrameData& frame) override;
    void present() override;
    void resize(int width, int height) override;

private:
    Window& m_window;
    SDL_GLContextState* m_context = nullptr;
    std::unique_ptr<GridPass> m_grid;
    int m_width = 0;
    int m_height = 0;
};

} // namespace yqlora
