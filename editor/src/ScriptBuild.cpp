#include "yqlora/editor/ScriptBuild.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/editor/CompilerDiagnostics.hpp"
#include "yqlora/editor/LogBuffer.hpp"
#include "yqlora/platform/Platform.hpp"
#include "yqlora/platform/Process.hpp"
#include "yqlora/project/Project.hpp"

#include <chrono>
#include <cstdint>
#include <format>
#include <fstream>
#include <sstream>
#include <utility>
#include <vector>

namespace yqlora::editor {

namespace {

namespace fs = std::filesystem;

// Запись пути к библиотеке, которую пишет yqlora_add_game_module (cmake/YqloraGameModule.cmake).
constexpr const char* kModuleRecordFile = "yqlora_game_module.txt";
// Так Ninja говорит, что всё собрано: в тихой сборке это не новость.
constexpr std::string_view kNothingToDo = "ninja: no work to do.";
// Меняется, когда меняется то, как настраивается папка сборки (например,
// короткие пути): старая папка тогда не годится, и сборка берёт новую.
constexpr std::string_view kBuildLayout = "ascii-paths\n";
// Пакет внутри SDK, см. cmake/Sdk.cmake.
constexpr const char* kSdkCmakeFolder = "cmake";

// FNV-1a: отпечаток одинаков на всех машинах и сборках редактора, в отличие от std::hash.
constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

std::uint64_t fnv1a(std::string_view text)
{
    std::uint64_t hash = kFnvOffset;
    for (const char c : text) {
        hash ^= static_cast<unsigned char>(c);
        hash *= kFnvPrime;
    }

    return hash;
}

std::string trimmed(std::string text)
{
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) {
        text.pop_back();
    }

    return text;
}

std::optional<fs::path> readModuleRecord(const fs::path& buildDirectory)
{
    std::ifstream file(buildDirectory / kModuleRecordFile, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    std::stringstream text;
    text << file.rdbuf();

    return fromUtf8(trimmed(text.str()));
}

} // namespace

fs::path scriptBuildDirectory(const ScriptBuildSetup& setup)
{
    const std::string key = std::string(kBuildLayout) + setup.toolchain.fingerprint() + toUtf8(setup.sdkDirectory);

    return setup.projectDirectory / Project::buildDirectoryName()
           / std::format("{}-{:016x}", setup.configuration, fnv1a(key));
}

std::optional<fs::path> builtGameLibrary(const ScriptBuildSetup& setup)
{
    auto library = readModuleRecord(scriptBuildDirectory(setup));
    std::error_code ec;
    if (!library || !fs::exists(*library, ec)) {
        return std::nullopt;
    }

    return library;
}

ScriptBuild::ScriptBuild(LogBuffer& log)
    : m_log(log)
{
}

ScriptBuild::~ScriptBuild()
{
    cancel();
}

void ScriptBuild::request(ScriptBuildSetup setup)
{
    const std::scoped_lock lock(m_mutex);
    if (m_running) {
        // Хоть один громкий запрос - и отложенная сборка громкая.
        setup.quiet = setup.quiet && (!m_pending || m_pending->quiet);
        m_pending = std::move(setup);

        return;
    }
    // Прошлый поток уже закончил (m_running сброшен), join не ждёт.
    if (m_worker.joinable()) {
        m_worker.join();
    }
    m_running = true;
    m_worker = std::jthread([this, setup = std::move(setup)](std::stop_token stop) mutable {
        run(std::move(setup), stop);
    });
}

bool ScriptBuild::building() const
{
    const std::scoped_lock lock(m_mutex);

    return m_running;
}

std::optional<ScriptBuildResult> ScriptBuild::takeResult()
{
    const std::scoped_lock lock(m_mutex);

    return std::exchange(m_result, std::nullopt);
}

void ScriptBuild::cancel()
{
    {
        const std::scoped_lock lock(m_mutex);
        m_pending.reset();
    }
    if (m_worker.joinable()) {
        m_worker.request_stop();
        m_worker.join();
    }
    const std::scoped_lock lock(m_mutex);
    m_running = false;
    m_result.reset();
}

void ScriptBuild::run(ScriptBuildSetup setup, std::stop_token stop)
{
    while (true) {
        ScriptBuildResult result = buildOnce(setup, stop);
        const std::scoped_lock lock(m_mutex);
        m_result = std::move(result);
        if (stop.stop_requested() || !m_pending) {
            m_running = false;

            return;
        }
        setup = std::move(*m_pending);
        m_pending.reset();
    }
}

ScriptBuildResult ScriptBuild::buildOnce(const ScriptBuildSetup& setup, std::stop_token stop)
{
    // Тихая сборка, которой нечего делать, не оставляет в логе ни строки:
    // начало пишется перед первым сообщением.
    bool announced = false;
    const auto add = [this](log::Level level, std::string message) {
        m_log.add({.level = level, .source = LogSource::Build, .message = std::move(message)});
    };
    const auto note = [&](log::Level level, std::string message) {
        if (!announced) {
            announced = true;
            add(log::Level::Info, std::format("Building scripts ({})", setup.configuration));
        }
        add(level, std::move(message));
    };
    const auto runTool = [&](std::vector<std::string> arguments) {
        const platform::ProcessOptions options{
            .program = setup.toolchain.cmake,
            .arguments = std::move(arguments),
            .workingDirectory = {},
        };
        const platform::ProcessResult result = platform::runProcess(
            options,
            [&](std::string_view line) {
                if (line != kNothingToDo) {
                    note(parseCompilerOutput(line).level, std::string(line));
                }
            },
            stop
        );
        if (!result.started) {
            note(log::Level::Error, std::format("Cannot run '{}': {}", toUtf8(options.program), result.error));
        }

        return result.succeeded();
    };

    const auto started = std::chrono::steady_clock::now();
    if (!setup.quiet) {
        announced = true;
        add(log::Level::Info, std::format("Building scripts ({})", setup.configuration));
    }
    ScriptBuildResult result;
    // vcvarsall.bat ставит окружение только в своём cmd: запуск CMake через
    // него ещё не сделан, поэтому движок, собранный MSVC, проекты пока не собирает.
    if (!setup.toolchain.vcvarsall.empty()) {
        note(log::Level::Error, "Building scripts with MSVC from the editor is not supported yet");

        return result;
    }
    const fs::path scripts = setup.projectDirectory / Project::scriptsDirectoryName();
    std::error_code ec;
    if (!fs::is_directory(scripts, ec)) {
        note(log::Level::Error, "The project has no scripts folder");

        return result;
    }

    const fs::path buildDirectory = scriptBuildDirectory(setup);
    fs::create_directories(buildDirectory, ec);
    // Ninja на Windows не находит файлы по путям не из ASCII и пересобирал бы
    // всё каждый раз: CMake и Ninja получают короткие пути (8.3).
    const auto scriptsAscii = platform::asciiPath(scripts);
    const auto buildAscii = platform::asciiPath(buildDirectory);
    const auto sdkAscii = platform::asciiPath(setup.sdkDirectory);
    const fs::path scriptsPath = scriptsAscii.value_or(scripts);
    const fs::path buildPath = buildAscii.value_or(buildDirectory);
    const fs::path sdkPath = sdkAscii.value_or(setup.sdkDirectory);
    if (!scriptsAscii || !buildAscii || !sdkAscii) {
        note(
            log::Level::Warning,
            "The project or editor path has non-ASCII characters and no short name: Ninja may rebuild "
            "everything every time. Move the project to a path in Latin letters"
        );
    }
    // Запись появляется, только когда CMake настроил папку до конца: после
    // неудачной настройки следующая сборка настроит её снова. Дальше CMake
    // перенастраивает сам, когда меняются CMakeLists.txt.
    if (!fs::exists(buildDirectory / kModuleRecordFile, ec)) {
        std::vector<std::string> arguments{
            "-S",
            toUtf8(scriptsPath),
            "-B",
            toUtf8(buildPath),
            "-G",
            setup.toolchain.generator,
            // C-компилятор передаётся всегда, а проект может быть только на C++.
            "--no-warn-unused-cli",
            "-DCMAKE_BUILD_TYPE=" + setup.configuration,
            "-Dyqlora_DIR=" + toUtf8(sdkPath / kSdkCmakeFolder),
        };
        if (!setup.toolchain.makeProgram.empty()) {
            arguments.push_back("-DCMAKE_MAKE_PROGRAM=" + toUtf8(setup.toolchain.makeProgram));
        }
        if (!setup.toolchain.cCompiler.empty()) {
            arguments.push_back("-DCMAKE_C_COMPILER=" + toUtf8(setup.toolchain.cCompiler));
        }
        arguments.push_back("-DCMAKE_CXX_COMPILER=" + toUtf8(setup.toolchain.cxxCompiler));
        if (!runTool(std::move(arguments))) {
            note(log::Level::Error, stop.stop_requested() ? "Build stopped" : "Configuring scripts failed");

            return result;
        }
    }
    const auto previous = readModuleRecord(buildDirectory);
    const auto previousTime = previous ? fs::last_write_time(*previous, ec) : fs::file_time_type{};
    if (!runTool({"--build", toUtf8(buildPath)})) {
        note(log::Level::Error, stop.stop_requested() ? "Build stopped" : "Building scripts failed");

        return result;
    }

    const auto library = readModuleRecord(buildDirectory);
    if (!library || !fs::exists(*library, ec)) {
        note(log::Level::Error, "The build did not produce the game module");

        return result;
    }
    result.succeeded = true;
    result.library = *library;
    result.changed = library != previous || fs::last_write_time(*library, ec) != previousTime;
    if (announced) {
        const std::chrono::duration<float> elapsed = std::chrono::steady_clock::now() - started;
        add(log::Level::Info, std::format("Scripts built in {:.1f} s", elapsed.count()));
    }

    return result;
}

} // namespace yqlora::editor
