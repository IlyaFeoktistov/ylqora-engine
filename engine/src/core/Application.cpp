#include "yqlora/core/Application.hpp"

#include "yqlora/core/FrameClock.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/LogSettings.hpp"
#include "yqlora/core/Settings.hpp"
#include "yqlora/core/UiLayer.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/i18n/Localization.hpp"
#include "yqlora/platform/InputFeeder.hpp"
#include "yqlora/platform/Platform.hpp"
#include "yqlora/platform/Window.hpp"
#include "yqlora/render/FrameBuilder.hpp"
#include "yqlora/render/Renderer.hpp"

#include <algorithm>
#include <exception>

#ifdef _WIN32
// На ноутбуках с двумя видеокартами драйвер по умолчанию запускает программу
// на встроенной. По этим экспортам из exe драйверы NVIDIA и AMD выбирают
// дискретную. Код приложения всегда подтягивает этот файл, поэтому экспорты
// попадают в любой exe на движке. Пользователь может переопределить выбор
// в настройках графики Windows.
extern "C" {
__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

namespace yqlora {

namespace {

constexpr const char* kSettingsFile = "settings.json";
constexpr const char* kUiLayoutFile = "imgui.ini";
constexpr const char* kAssetsFolder = "assets";
constexpr const char* kLocalesFolder = "locales";
// Пока окно ещё не нарисовано интерфейсом.
constexpr Vec4 kWindowClearColor{0.1f, 0.1f, 0.11f, 1.0f};
// Самый длинный шаг кадра: после паузы в отладчике не прыгаем на секунды вперёд.
constexpr double kMaxFrameSeconds = 0.1;

} // namespace

Application::Application(ApplicationConfig config)
    : m_config(std::move(config))
{
    registerEngineTypes(m_registry);
    m_scene = std::make_unique<Scene>(m_services);
}

Application::~Application() = default;

int Application::run()
{
    log::openFile(userDataDirectory() / "yqlora.log");
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
        if (m_config.ui) {
            m_ui = std::make_unique<UiLayer>(*m_renderer, userDataDirectory() / kUiLayoutFile);
        }

        onStart();
        mainLoop();
        onShutdown();
    } catch (const std::exception& e) {
        log::error("Fatal: {}", e.what());
        exitCode = 1;
    }

    m_ui.reset();

    // Рендерер держит контекст окна, поэтому уничтожаем строго в обратном порядке.
    m_renderer.reset();
    m_window.reset();
    Localization::setActive(nullptr);
    platform::shutdown();

    return exitCode;
}

void Application::initSettingsAndLanguage()
{
    const auto settingsFile = userDataDirectory() / kSettingsFile;
    m_settings = std::make_unique<Settings>(settingsFile);
    m_settings->load();
    log::info("Settings: {}", toUtf8(settingsFile));

    m_localization = std::make_unique<Localization>(assetsDirectory() / kLocalesFolder);
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
    updateTitle();

    return true;
}

std::filesystem::path Application::userDataDirectory() const
{
    return platform::userDataPath(organization(), m_config.name);
}

std::filesystem::path Application::assetsDirectory() const
{
    return platform::executableDir() / kAssetsFolder;
}

void Application::setTitleDetail(std::string detail)
{
    m_titleDetail = std::move(detail);
    updateTitle();
}

void Application::updateTitle()
{
    if (!m_window) {
        return;
    }
    const std::string title = tr(m_config.titleKey);
    m_window->setTitle(m_titleDetail.empty() ? title : m_titleDetail + " - " + title);
}

void Application::showFolderDialog(
    const std::filesystem::path& start,
    std::function<void(std::optional<std::filesystem::path>)> callback
)
{
    m_window->showFolderDialog(start, std::move(callback));
}

void Application::setMouseWrapping(bool wrapping)
{
    m_window->setMouseWrapping(wrapping);
}

Vec2 Application::windowSize() const
{
    return m_window->sizeInPoints();
}

float Application::aspectRatio() const
{
    const int width = m_window->widthInPixels();
    const int height = std::max(m_window->heightInPixels(), 1);

    return static_cast<float>(width) / static_cast<float>(height);
}

DepthRange Application::depthRange() const
{
    return m_renderer->depthRange();
}

void Application::mainLoop()
{
    WindowEvents events;
    events.onCloseRequested = [this] {
        onCloseRequested();
    };
    events.onResized = [this](int width, int height) {
        m_renderer->resize(width, height);
    };

    InputFeeder input(m_input);
    m_running = true;
    FrameClock clock(kMaxFrameSeconds);
    clock.reset(platform::timeSeconds());

    while (m_running) {
        const float deltaSeconds = clock.tick(platform::timeSeconds());

        input.beginFrame();
        m_window->pollEvents(events, input);
        if (m_ui && m_window->mouseWrapping()) {
            m_ui->setMouseDelta(m_input.mousePosition(), m_input.mouseDelta());
        }
        if (!m_running) {
            break;
        }

        if (m_sceneRunning) {
            m_scene->advance(deltaSeconds);
        }
        onUpdate(deltaSeconds);
        // Сцену меняют и без игры: редактор правит её между кадрами.
        m_scene->updateTransforms();

        if (m_ui) {
            m_ui->beginFrame();
            onGui();
            m_renderer->clearWindow(kWindowClearColor);
            m_ui->endFrame();
        } else {
            FrameData frame = extractFrame(*m_scene, aspectRatio(), depthRange());
            onPrepareFrame(frame);
            m_renderer->render(frame);
        }
        m_renderer->present();
    }
}

} // namespace yqlora
