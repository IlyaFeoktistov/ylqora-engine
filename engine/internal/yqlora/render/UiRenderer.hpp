#pragma once

namespace yqlora {

// Отрисовка Dear ImGui для графического API. Отдельно от Renderer: тот рисует
// сцену, а интерфейс нужен редактору и инструментам, игре он не обязателен.
// Создаётся рендерером (Renderer::createUiRenderer) после ImGui::CreateContext
// и удаляется до ImGui::DestroyContext.
class UiRenderer {
public:
    virtual ~UiRenderer() = default;

    // До ImGui::NewFrame().
    virtual void beginFrame() = 0;

    // Рисует собранный после ImGui::Render() интерфейс поверх окна.
    virtual void render() = 0;
};

} // namespace yqlora
