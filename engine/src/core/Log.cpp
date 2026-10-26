#include "yqlora/core/Log.hpp"

#include "yqlora/core/LogFile.hpp"

#include <cstdio>
#include <fstream>
#include <mutex>

namespace yqlora::log {

namespace {

// Лог могут писать и из других потоков: например, ответ системного диалога.
std::mutex g_mutex;
std::ofstream g_file;

const char* prefix(Level level)
{
    switch (level) {
    case Level::Info: return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error: return "ERROR";
    }

    return "INFO";
}

} // namespace

void openFile(const std::filesystem::path& file)
{
    const std::lock_guard lock(g_mutex);
    g_file.open(file, std::ios::out | std::ios::trunc);
}

void write(Level level, std::string_view message)
{
    const std::lock_guard lock(g_mutex);
    std::fprintf(stderr, "[yqlora][%s] %.*s\n", prefix(level), static_cast<int>(message.size()), message.data());
    if (g_file.is_open()) {
        g_file << '[' << prefix(level) << "] " << message << std::endl;
    }
}

} // namespace yqlora::log
