#include "yqlora/platform/Platform.hpp"
#include "yqlora/core/Utf8Path.hpp"

#include "yqlora/core/Log.hpp"

#include <SDL3/SDL.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <cstdio>
#endif

namespace yqlora::platform {

namespace {

constexpr double kNanosecondsPerSecond = 1e9;

} // namespace

void openConsole()
{
#ifdef _WIN32
    if (!AttachConsole(ATTACH_PARENT_PROCESS) && !AllocConsole()) {
        return;
    }
    FILE* stream = nullptr;
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);
    // Лог в UTF-8, иначе кириллица в путях превратится в кракозябры.
    SetConsoleOutputCP(CP_UTF8);
#endif
}

bool initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log::error("SDL_Init failed: {}", SDL_GetError());

        return false;
    }
    log::info(
        "Platform: SDL {}.{}.{}, video driver '{}'",
        SDL_MAJOR_VERSION,
        SDL_MINOR_VERSION,
        SDL_MICRO_VERSION,
        SDL_GetCurrentVideoDriver()
    );

    return true;
}

void shutdown()
{
    SDL_Quit();
}

std::filesystem::path executableDir()
{
    const char* base = SDL_GetBasePath();

    return base ? fromUtf8(base) : std::filesystem::current_path();
}

std::filesystem::path userDataPath(std::string_view organization, std::string_view application)
{
    char* path = SDL_GetPrefPath(std::string(organization).c_str(), std::string(application).c_str());
    if (!path) {
        log::warn("SDL_GetPrefPath failed: {}, using current directory", SDL_GetError());

        return std::filesystem::current_path();
    }
    std::filesystem::path result = fromUtf8(path);
    SDL_free(path);

    return result;
}

std::filesystem::path documentsPath()
{
    if (const char* documents = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS)) {
        return fromUtf8(documents);
    }
    if (const char* home = SDL_GetUserFolder(SDL_FOLDER_HOME)) {
        return fromUtf8(home);
    }

    return std::filesystem::current_path();
}

std::vector<std::string> preferredLanguages()
{
    std::vector<std::string> result;
    int count = 0;
    SDL_Locale** locales = SDL_GetPreferredLocales(&count);
    if (!locales) {
        return result;
    }
    for (int i = 0; i < count; ++i) {
        if (locales[i] && locales[i]->language) {
            result.emplace_back(locales[i]->language);
        }
    }
    SDL_free(locales);

    return result;
}

double timeSeconds()
{
    return static_cast<double>(SDL_GetTicksNS()) / kNanosecondsPerSecond;
}

} // namespace yqlora::platform
