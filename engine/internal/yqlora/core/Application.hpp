#pragma once

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/core/Export.hpp"
#include "yqlora/input/Input.hpp"
#include "yqlora/platform/Window.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/render/FrameData.hpp"
#include "yqlora/render/GraphicsApi.hpp"
#include "yqlora/scene/Scene.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace yqlora {

class Window;
class Renderer;
class UiLayer;
class Settings;
class Localization;

struct ApplicationConfig {
    // Латиница, без пробелов: из имени строится путь к папке настроек.
    std::string name = "YqloraApp";
    // Ключ перевода для заголовка окна.
    std::string titleKey = "app.title";
    int width = kDefaultWindowWidth;
    int height = kDefaultWindowHeight;
    GraphicsApi api = GraphicsApi::OpenGL;
    // Интерфейс на Dear ImGui. С ним приложение само решает, куда рисовать
    // сцену (обычно в RenderTarget внутри окна интерфейса) в onGui().
    bool ui = false;
};

// Владеет окном, рендерером, сценой, настройками и локализацией,
// крутит игровой цикл. Игра наследуется и переопределяет on*-методы.
class YQLORA_API Application {
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

    // Типы узлов и компонентов: движка (регистрируются сразу) и игры.
    TypeRegistry& registry()
    {
        return m_registry;
    }

    const TypeRegistry& registry() const
    {
        return m_registry;
    }

    // Службы для сцен приложения: ввод, реестр, открытый проект.
    EngineServices& services()
    {
        return m_services;
    }

    Settings& settings()
    {
        return *m_settings;
    }

    Localization& localization()
    {
        return *m_localization;
    }

    const Input& input() const
    {
        return m_input;
    }

    // См. Window::setMouseWrapping.
    void setMouseWrapping(bool wrapping);

    // Размер окна в тех же единицах, что положение мыши.
    Vec2 windowSize() const;

    // Ширина к высоте области вывода и диапазон глубины рендерера:
    // нужны, чтобы построить проекцию своей камеры.
    float aspectRatio() const;
    DepthRange depthRange() const;

    Renderer& renderer()
    {
        return *m_renderer;
    }

    // Обновлять ли сцену каждый кадр. У игры всегда да; редактор включает
    // обновление только на время Play.
    void setSceneRunning(bool running)
    {
        m_sceneRunning = running;
    }

    // Переключает язык интерфейса и сохраняет выбор в настройках.
    bool setLanguage(std::string_view language);

    // Название разработчика движка: из него строятся папки в профиле пользователя.
    static constexpr std::string_view organization()
    {
        return "Yqlora";
    }

    // Папка для данных приложения в профиле пользователя (настройки, списки).
    std::filesystem::path userDataDirectory() const;

    // Ресурсы движка рядом с программой: переводы, шрифты, темы.
    std::filesystem::path assetsDirectory() const;

    // Уточнение в заголовке окна перед названием программы, например имя проекта.
    // Пустая строка - только название.
    void setTitleDetail(std::string detail);

    // См. Window::showFolderDialog.
    void showFolderDialog(
        const std::filesystem::path& start,
        std::function<void(std::optional<std::filesystem::path>)> callback
    );

protected:
    virtual void onStart()
    {
    }

    virtual void onUpdate(float /*deltaSeconds*/)
    {
    }

    // Последняя правка кадра перед отрисовкой, например включить сетку редактора.
    virtual void onPrepareFrame(FrameData& /*frame*/)
    {
    }

    // Окно просят закрыть (крестик, Alt+F4). По умолчанию сразу выход;
    // приложение может сначала спросить про несохранённые изменения.
    virtual void onCloseRequested()
    {
        quit();
    }

    // Только при ApplicationConfig::ui: строит интерфейс кадра вызовами ImGui.
    // Шрифт и тему стоит задать в onStart, до первого кадра.
    virtual void onGui()
    {
    }

    virtual void onShutdown()
    {
    }

private:
    void initSettingsAndLanguage();
    void updateTitle();
    void mainLoop();

    ApplicationConfig m_config;
    TypeRegistry m_registry;
    Input m_input;
    EngineServices m_services{m_input, m_registry};
    Scene m_scene{m_services};
    std::unique_ptr<Settings> m_settings;
    std::unique_ptr<Localization> m_localization;
    std::unique_ptr<Window> m_window;
    std::unique_ptr<Renderer> m_renderer;
    // Только при ApplicationConfig::ui.
    std::unique_ptr<UiLayer> m_ui;
    bool m_running = false;
    bool m_sceneRunning = true;
    std::string m_titleDetail;
};

} // namespace yqlora
