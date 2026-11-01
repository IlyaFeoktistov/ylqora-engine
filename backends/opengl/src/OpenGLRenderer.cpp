#include "yqlora/opengl/OpenGLRenderer.hpp"

#include "yqlora/opengl/GlMesh.hpp"
#include "yqlora/opengl/GlRenderTarget.hpp"
#include "yqlora/opengl/GlTexture.hpp"
#include "yqlora/opengl/GlUiRenderer.hpp"
#include "yqlora/opengl/GridPass.hpp"
#include "yqlora/opengl/MeshPass.hpp"
#include "yqlora/opengl/OutlinePass.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/platform/Window.hpp"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <stdexcept>
#include <string>

namespace yqlora::opengl {

namespace {

constexpr int kDepthBits = 24;
// Трафарет нужен контуру выделения.
constexpr int kStencilBits = 8;
// Кадр без единого вида (в сцене нет активной камеры).
const Vec4 kNoViewColor(0.0f, 0.0f, 0.0f, 1.0f);

void clearView(CameraClear clear, const Vec4& background)
{
    switch (clear) {
    case CameraClear::ColorAndDepth:
        glClearColor(background.r, background.g, background.b, background.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        break;
    case CameraClear::DepthOnly: glClear(GL_DEPTH_BUFFER_BIT); break;
    case CameraClear::Nothing: break;
    }
}

} // namespace

void OpenGLRenderer::prepareWindow()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, kVersionMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, kVersionMinor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, kDepthBits);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, kStencilBits);
}

OpenGLRenderer::OpenGLRenderer(Window& window)
    : m_window(window)
{
    m_context = SDL_GL_CreateContext(window.sdlWindow());
    if (!m_context) {
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    }

    const int version = gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress));
    if (version == 0) {
        SDL_GL_DestroyContext(m_context);
        throw std::runtime_error("Failed to load OpenGL functions");
    }

    log::info(
        "OpenGL {}.{}: {} ({})",
        GLAD_VERSION_MAJOR(version),
        GLAD_VERSION_MINOR(version),
        reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
        reinterpret_cast<const char*>(glGetString(GL_VERSION))
    );

    glEnable(GL_DEPTH_TEST);
    m_meshes = std::make_unique<GlMeshCache>();
    m_uiTextures = std::make_unique<GpuResourceCache<GlTexture>>();
    m_meshPass = std::make_unique<MeshPass>(*m_meshes);
    m_grid = std::make_unique<GridPass>();
    m_outline = std::make_unique<OutlinePass>(*m_meshes);
    resize(window.widthInPixels(), window.heightInPixels());
}

OpenGLRenderer::~OpenGLRenderer()
{
    // GL-объекты удаляются, пока контекст ещё жив.
    m_outline.reset();
    m_grid.reset();
    m_meshPass.reset();
    m_meshes.reset();
    m_uiTextures.reset();
    SDL_GL_DestroyContext(m_context);
}

void OpenGLRenderer::render(const FrameData& frame)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    drawScene(frame, m_width, m_height);
}

void OpenGLRenderer::render(const FrameData& frame, RenderTarget& target)
{
    // Цели создаёт только этот рендерер, так что тип известен.
    auto& glTarget = static_cast<GlRenderTarget&>(target);
    glBindFramebuffer(GL_FRAMEBUFFER, glTarget.framebuffer());
    drawScene(frame, glTarget.width(), glTarget.height());
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::unique_ptr<RenderTarget> OpenGLRenderer::createRenderTarget(int width, int height)
{
    return std::make_unique<GlRenderTarget>(width, height);
}

void OpenGLRenderer::clearWindow(const Vec4& color)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

std::unique_ptr<UiRenderer> OpenGLRenderer::createUiRenderer()
{
    return std::make_unique<GlUiRenderer>(m_window.sdlWindow(), m_context);
}

void OpenGLRenderer::drawScene(const FrameData& frame, int width, int height)
{
    // Отрисовка интерфейса меняет состояние GL, возвращаем нужное сцене.
    glDisable(GL_SCISSOR_TEST);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);
    if (frame.views.empty()) {
        clearView(CameraClear::ColorAndDepth, kNoViewColor);

        return;
    }
    // Виды по очереди: следующая камера рисует поверх предыдущей.
    for (const ViewData& view : frame.views) {
        clearView(view.clear, view.background);
        m_meshPass->draw(frame, view);
    }

    // Сетка полупрозрачная, поэтому рисуется после объектов сцены.
    const CameraData& top = frame.views.back().camera;
    if (frame.editor.grid) {
        m_grid->draw(top);
    }
    m_outline->draw(frame, top, width, height);
}

void OpenGLRenderer::present()
{
    SDL_GL_SwapWindow(m_window.sdlWindow());
    m_meshes->endFrame();
    m_uiTextures->endFrame();
}

UiTexture OpenGLRenderer::uiTexture(ResourceId id, std::uint64_t revision, const TextureData& data)
{
    const GlTexture& texture = m_uiTextures->get(id, revision, [&] {
        return std::make_unique<GlTexture>(data, GlTexture::Use::Interface);
    });

    // Первая строка картинки лежит в текстуре при v = 0 (см. GlTexture).
    return {texture.id(), {0.0f, 0.0f}, {1.0f, 1.0f}};
}

bool OpenGLRenderer::setVsync(bool enabled)
{
    if (!SDL_GL_SetSwapInterval(enabled ? 1 : 0)) {
        log::warn("VSync cannot be switched: {}", SDL_GetError());

        return false;
    }

    return true;
}

void OpenGLRenderer::resize(int width, int height)
{
    m_width = width;
    m_height = height;
}

} // namespace yqlora::opengl
