#include "yqlora/ui/UiLayer.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/render/Renderer.hpp"
#include "yqlora/render/UiRenderer.hpp"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl3.h>

namespace yqlora {

UiLayer::UiLayer(Renderer& renderer, const std::filesystem::path& layoutFile)
    : m_layoutFile(toUtf8(layoutFile))
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = m_layoutFile.c_str();

    // Шрифт и цвета задаёт само приложение (у редактора это файл темы),
    // движок свой внешний вид не навязывает.
    attachRenderer(renderer);
}

UiLayer::~UiLayer()
{
    // Отрисовщик держит ресурсы ImGui и уходит раньше контекста.
    m_renderer.reset();
    ImGui::DestroyContext();
}

void UiLayer::releaseRenderer()
{
    m_renderer.reset();
}

void UiLayer::attachRenderer(Renderer& renderer)
{
    m_renderer = renderer.createUiRenderer();
}

void UiLayer::setMouseDelta(const Vec2& position, const Vec2& delta)
{
    const Vec2 previous = position - delta;
    ImGui::GetIO().MousePosPrev = ImVec2(previous.x, previous.y);
}

void UiLayer::processEvent(const SDL_Event& event)
{
    ImGui_ImplSDL3_ProcessEvent(&event);
}

void UiLayer::beginFrame()
{
    m_renderer->beginFrame();
    ImGui::NewFrame();
}

void UiLayer::endFrame()
{
    ImGui::Render();
    m_renderer->render();
}

} // namespace yqlora
