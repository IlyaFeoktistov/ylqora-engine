#include "yqlora/platform/Platform.hpp"

#include "yqlora/core/Log.hpp"

#include <SDL3/SDL.h>

namespace yqlora::platform {

bool initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        log::error("SDL_Init failed: {}", SDL_GetError());
        return false;
    }
    log::info("Platform: SDL {}.{}.{}, video driver '{}'", SDL_MAJOR_VERSION, SDL_MINOR_VERSION,
              SDL_MICRO_VERSION, SDL_GetCurrentVideoDriver());
    return true;
}

void shutdown()
{
    SDL_Quit();
}

std::filesystem::path executableDir()
{
    const char* base = SDL_GetBasePath();
    return base ? std::filesystem::path(reinterpret_cast<const char8_t*>(base)) : std::filesystem::current_path();
}

std::filesystem::path userDataPath(std::string_view organization, std::string_view application)
{
    char* path = SDL_GetPrefPath(std::string(organization).c_str(), std::string(application).c_str());
    if (!path) {
        log::warn("SDL_GetPrefPath failed: {}, using current directory", SDL_GetError());
        return std::filesystem::current_path();
    }
    std::filesystem::path result(reinterpret_cast<const char8_t*>(path));
    SDL_free(path);
    return result;
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
    return static_cast<double>(SDL_GetTicksNS()) / 1'000'000'000.0;
}

} // namespace yqlora::platform
