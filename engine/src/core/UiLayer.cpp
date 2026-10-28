#include "yqlora/core/UiLayer.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/render/Renderer.hpp"
#include "yqlora/render/UiRenderer.hpp"

#include <imgui.h>

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
    m_renderer = renderer.createUiRenderer();
}

UiLayer::~UiLayer()
{
    // Отрисовщик держит ресурсы ImGui и уходит раньше контекста.
    m_renderer.reset();
    ImGui::DestroyContext();
}

void UiLayer::setMouseDelta(const Vec2& position, const Vec2& delta)
{
    const Vec2 previous = position - delta;
    ImGui::GetIO().MousePosPrev = ImVec2(previous.x, previous.y);
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
