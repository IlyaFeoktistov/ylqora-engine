#pragma once

#include "yqlora/math/Math.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace yqlora {

class Renderer;
class UiRenderer;

// Интерфейс на Dear ImGui поверх окна: контекст ImGui, файл раскладки окон
// и отрисовка под графический API рендерера. Нужен редактору и инструментам,
// игре без интерфейса он не создаётся.
class UiLayer {
public:
    // layoutFile - где ImGui хранит раскладку окон.
    UiLayer(Renderer& renderer, const std::filesystem::path& layoutFile);
    ~UiLayer();

    UiLayer(const UiLayer&) = delete;
    UiLayer& operator=(const UiLayer&) = delete;

    // Курсор перенесли к другому краю окна (см. Window::setMouseWrapping).
    // ImGui сам считает смещение мыши по положению курсора, и перенос выглядел
    // бы для него рывком через всё окно. Подставляет прошлое положение так,
    // чтобы смещение совпало с уже очищенным от переносов.
    void setMouseDelta(const Vec2& position, const Vec2& delta);

    // Между ними приложение строит интерфейс кадра вызовами ImGui.
    void beginFrame();
    // Рисует интерфейс поверх окна.
    void endFrame();

private:
    // ImGui хранит только указатель на строку с путём, поэтому она живёт здесь.
    std::string m_layoutFile;
    std::unique_ptr<UiRenderer> m_renderer;
};

} // namespace yqlora
