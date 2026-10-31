#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>
#include <string>
#include <string_view>

struct SDL_SharedObject;

namespace yqlora::platform {

// Разделяемая библиотека (.so, .dll), загруженная во время работы. Выгружается
// в деструкторе: к этому моменту ни одной функции и ни одного объекта из неё
// не должно остаться.
class YQLORA_API SharedLibrary {
public:
    // Не загрузилась - loaded() false, причина в error().
    explicit SharedLibrary(const std::filesystem::path& path);
    ~SharedLibrary();

    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator=(const SharedLibrary&) = delete;

    bool loaded() const
    {
        return m_handle != nullptr;
    }

    const std::string& error() const
    {
        return m_error;
    }

    // Экспортированная функция; nullptr, если её нет. Вызывают её, приведя к
    // настоящему типу через reinterpret_cast.
    using Function = void (*)();
    Function function(const char* name) const;

private:
    SDL_SharedObject* m_handle = nullptr;
    std::string m_error;
};

// Имя файла библиотеки name по правилам системы: game.dll на Windows,
// libgame.so на Linux, libgame.dylib на macOS.
YQLORA_API std::string sharedLibraryFileName(std::string_view name);

} // namespace yqlora::platform
