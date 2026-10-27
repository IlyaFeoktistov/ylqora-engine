#pragma once

#include "yqlora/render/UiRenderer.hpp"

struct SDL_GLContextState;
struct SDL_Window;

namespace yqlora {

class GlUiRenderer final : public UiRenderer {
public:
    GlUiRenderer(SDL_Window* window, SDL_GLContextState* context);
    ~GlUiRenderer() override;

    GlUiRenderer(const GlUiRenderer&) = delete;
    GlUiRenderer& operator=(const GlUiRenderer&) = delete;

    void beginFrame() override;
    void render() override;
};

} // namespace yqlora
