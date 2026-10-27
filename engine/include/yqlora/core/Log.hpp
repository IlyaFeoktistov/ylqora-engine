#pragma once

#include "yqlora/core/Export.hpp"

#include <format>
#include <string_view>

namespace yqlora::log {

// По возрастанию важности.
enum class Level {
    Info,
    Warning,
    Error,
};

// Пишется ли сейчас уровень level. Отключённые сообщения не форматируются.
YQLORA_API bool isEnabled(Level level);

YQLORA_API void write(Level level, std::string_view message);

template <typename... Args>
void info(std::format_string<Args...> fmt, Args&&... args)
{
    if (isEnabled(Level::Info)) {
        write(Level::Info, std::format(fmt, std::forward<Args>(args)...));
    }
}

template <typename... Args>
void warn(std::format_string<Args...> fmt, Args&&... args)
{
    if (isEnabled(Level::Warning)) {
        write(Level::Warning, std::format(fmt, std::forward<Args>(args)...));
    }
}

template <typename... Args>
void error(std::format_string<Args...> fmt, Args&&... args)
{
    if (isEnabled(Level::Error)) {
        write(Level::Error, std::format(fmt, std::forward<Args>(args)...));
    }
}

} // namespace yqlora::log
