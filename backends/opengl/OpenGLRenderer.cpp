#include "OpenGLRenderer.hpp"

#include "GlMesh.hpp"
#include "GridPass.hpp"
#include "MeshPass.hpp"
#include "OutlinePass.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/platform/Window.hpp"

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include <stdexcept>
#include <string>

namespace yqlora {

void OpenGLRenderer::prepareWindow()
{
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, kVersionMajor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, kVersionMinor);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
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

    // VSync: не крутим кадры быстрее монитора. Не везде поддерживается, это не критично.
    if (!SDL_GL_SetSwapInterval(1)) {
        log::warn("VSync is not available: {}", SDL_GetError());
    }

    log::info("OpenGL {}.{}: {} ({})", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version),
              reinterpret_cast<const char*>(glGetString(GL_RENDERER)),
              reinterpret_cast<const char*>(glGetString(GL_VERSION)));

    glEnable(GL_DEPTH_TEST);
    m_meshes = std::make_unique<GlMeshCache>();
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
    SDL_GL_DestroyContext(m_context);
}

void OpenGLRenderer::render(const FrameData& frame)
{
    glViewport(0, 0, m_width, m_height);
    glClearColor(frame.clearColor.r, frame.clearColor.g, frame.clearColor.b, frame.clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_meshPass->draw(frame);

    // Сетка полупрозрачная, поэтому рисуется после объектов сцены.
    if (frame.editor.grid) {
        m_grid->draw(frame.camera);
    }
    m_outline->draw(frame, m_width, m_height);
}

void OpenGLRenderer::present()
{
    SDL_GL_SwapWindow(m_window.sdlWindow());
}

void OpenGLRenderer::resize(int width, int height)
{
    m_width = width;
    m_height = height;
}

} // namespace yqlora
