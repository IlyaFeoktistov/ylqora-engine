#pragma once

#include "yqlora/input/Key.hpp"
#include "yqlora/render/GraphicsApi.hpp"
#include "yqlora/scene/Scene.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace yqlora {

class Window;
class Renderer;
class Settings;
class Localization;

struct ApplicationConfig {
    // Латиница, без пробелов: из имени строится путь к папке настроек.
    std::string name = "YqloraApp";
    // Ключ перевода для заголовка окна.
    std::string titleKey = "app.title";
    int width = 1280;
    int height = 720;
    GraphicsApi api = GraphicsApi::OpenGL;
};

// Владеет окном, рендерером, сценой, настройками и локализацией,
// крутит игровой цикл. Игра наследуется и переопределяет on*-методы.
class Application {
public:
    explicit Application(ApplicationConfig config);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Запускает цикл до закрытия окна. Возвращает код завершения процесса.
    int run();
    void quit()
    {
        m_running = false;
    }

    Scene& scene()
    {
        return m_scene;
    }

    Settings& settings()
    {
        return *m_settings;
    }

    Localization& localization()
    {
        return *m_localization;
    }

    // Переключает язык интерфейса и сохраняет выбор в настройках.
    bool setLanguage(std::string_view language);

protected:
    virtual void onStart()
    {
    }

    virtual void onUpdate(float /*deltaSeconds*/)
    {
    }

    virtual void onKeyPressed(Key /*key*/)
    {
    }

    virtual void onShutdown()
    {
    }

private:
    void initSettingsAndLanguage();
    void mainLoop();

    ApplicationConfig m_config;
    Scene m_scene;
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<Localization> m_localization;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    bool m_running = false;
};

} // namespace yqlora
