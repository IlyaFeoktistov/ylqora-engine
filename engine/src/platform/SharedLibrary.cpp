#include "yqlora/platform/SharedLibrary.hpp"

#include "yqlora/core/Utf8Path.hpp"

#include <SDL3/SDL.h>

namespace yqlora::platform {

SharedLibrary::SharedLibrary(const std::filesystem::path& path)
    : m_handle(SDL_LoadObject(toUtf8(path).c_str()))
{
    if (!m_handle) {
        m_error = SDL_GetError();
    }
}

SharedLibrary::~SharedLibrary()
{
    if (m_handle) {
        SDL_UnloadObject(m_handle);
    }
}

SharedLibrary::Function SharedLibrary::function(const char* name) const
{
    return m_handle ? SDL_LoadFunction(m_handle, name) : nullptr;
}

std::string sharedLibraryFileName(std::string_view name)
{
#if defined(_WIN32)
    return std::string(name) + ".dll";
#elif defined(__APPLE__)
    return "lib" + std::string(name) + ".dylib";
#else
    return "lib" + std::string(name) + ".so";
#endif
}

} // namespace yqlora::platform
