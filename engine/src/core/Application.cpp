#include "yqlora/core/Application.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Settings.hpp"
#include "yqlora/i18n/Localization.hpp"
#include "yqlora/platform/Platform.hpp"
#include "yqlora/platform/Window.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"

#include <algorithm>
#include <exception>

namespace yqlora {

namespace {

constexpr std::string_view kOrganization = "Yqlora";

} // namespace

Application::Application(ApplicationConfig config)
    : m_config(std::move(config))
{
}

Application::~Application() = default;

int Application::run()
{
    if (!platform::initialize()) {
        return 1;
    }

    int exitCode = 0;
    try {
        initSettingsAndLanguage();

        prepareWindowForApi(m_config.api);
        m_window = std::make_unique<Window>(WindowDesc{
            .title = tr(m_config.titleKey),
            .width = m_config.width,
            .height = m_config.height,
            .api = m_config.api,
        });
        m_renderer = createRenderer(m_config.api, *m_window);

        onStart();
        mainLoop();
        onShutdown();
    } catch (const std::exception& e) {
        log::error("Fatal: {}", e.what());
        exitCode = 1;
    }

    // Рендерер держит контекст окна, поэтому уничтожаем строго в обратном порядке.
    m_renderer.reset();
    m_window.reset();
    Localization::setActive(nullptr);
    platform::shutdown();

    return exitCode;
}

void Application::initSettingsAndLanguage()
{
    const auto settingsFile = platform::userDataPath(kOrganization, m_config.name) / "settings.json";
    m_settings = std::make_unique<Settings>(settingsFile);
    m_settings->load();
    log::info("Settings: {}", settingsFile.string());

    m_localization = std::make_unique<Localization>(platform::executableDir() / "assets" / "locales");
    Localization::setActive(m_localization.get());

    const auto available = m_localization->availableLanguages();
    const bool savedIsValid = std::ranges::find(available, m_settings->language) != available.end();

    if (savedIsValid) {
        m_localization->load(m_settings->language);

        return;
    }

    // Первый запуск (или язык из настроек больше не поддерживается):
    // берём язык системы и сразу запоминаем его, чтобы выбор не потерялся.
    const auto systemLanguages = platform::preferredLanguages();
    setLanguage(Localization::pickLanguage(systemLanguages, available));
}

bool Application::setLanguage(std::string_view language)
{
    if (!m_localization->load(language)) {
        return false;
    }
    m_settings->language = std::string(language);
    m_settings->save();
    if (m_window) {
        m_window->setTitle(tr(m_config.titleKey));
    }

    return true;
}

void Application::mainLoop()
{
    const WindowEvents events{
        .onCloseRequested = [this] {
            quit();
        },
        .onResized = [this](int width, int height) {
            m_renderer->resize(width, height);
        },
        .onKeyPressed = [this](Key key) {
            onKeyPressed(key);
        },
    };

    m_running = true;
    double previousTime = platform::timeSeconds();

    while (m_running) {
        const double now = platform::timeSeconds();
        // Ограничиваем шаг: после паузы в отладчике не прыгаем на секунды вперёд.
        const auto deltaSeconds = static_cast<float>(std::min(now - previousTime, 0.1));
        previousTime = now;

        m_window->pollEvents(events);
        if (!m_running) {
            break;
        }

        m_scene.update(deltaSeconds);
        onUpdate(deltaSeconds);

        const int width = m_window->widthInPixels();
        const int height = std::max(m_window->heightInPixels(), 1);
        const float aspect = static_cast<float>(width) / static_cast<float>(height);

        m_renderer->render(buildFrameData(m_scene, aspect, m_renderer->depthRange()));
    }
}

} // namespace yqlora
