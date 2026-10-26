#include "yqlora/render/Renderer.hpp"

#if YQLORA_HAS_OPENGL
#include "OpenGLRenderer.hpp"
#endif

#include <stdexcept>
#include <string>

namespace yqlora {

namespace {

[[noreturn]] void throwNotBuilt(GraphicsApi api)
{
    throw std::runtime_error(std::string("Renderer backend is not built: ") + std::string(toString(api)));
}

} // namespace

void prepareWindowForApi(GraphicsApi api)
{
    switch (api) {
    case GraphicsApi::OpenGL:
#if YQLORA_HAS_OPENGL
        OpenGLRenderer::prepareWindow();

        return;
#else
        break;
#endif
    case GraphicsApi::Vulkan:
    case GraphicsApi::Direct3D12:
        break;
    }
    throwNotBuilt(api);
}

std::unique_ptr<Renderer> createRenderer(GraphicsApi api, [[maybe_unused]] Window& window)
{
    switch (api) {
    case GraphicsApi::OpenGL:
#if YQLORA_HAS_OPENGL

        return std::make_unique<OpenGLRenderer>(window);
#else
        break;
#endif
    case GraphicsApi::Vulkan:
    case GraphicsApi::Direct3D12:
        break;
    }
    throwNotBuilt(api);
}

} // namespace yqlora
