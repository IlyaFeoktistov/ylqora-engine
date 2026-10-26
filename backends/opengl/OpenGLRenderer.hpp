#pragma once

#include "yqlora/render/Renderer.hpp"

#include <memory>

// Намеренно без glad/SDL: этот заголовок включает фабрика рендереров.
struct SDL_GLContextState;

namespace yqlora {

class GlMeshCache;
class GridPass;
class MeshPass;
class OutlinePass;

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
    void render(const FrameData& frame, RenderTarget& target) override;
    std::unique_ptr<RenderTarget> createRenderTarget(int width, int height) override;
    void clearWindow(const Vec4& color) override;

    void initUi() override;
    void shutdownUi() override;
    void beginUiFrame() override;
    void renderUi() override;

    void present() override;
    void resize(int width, int height) override;

private:
    // Рисует сцену в привязанный сейчас framebuffer размером width x height.
    void drawScene(const FrameData& frame, int width, int height);

    Window& m_window;
    SDL_GLContextState* m_context = nullptr;
    std::unique_ptr<GlMeshCache> m_meshes;
    std::unique_ptr<MeshPass> m_meshPass;
    std::unique_ptr<GridPass> m_grid;
    std::unique_ptr<OutlinePass> m_outline;
    int m_width = 0;
    int m_height = 0;
};

} // namespace yqlora
