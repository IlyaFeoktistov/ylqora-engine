#include "yqlora/core/Log.hpp"

#include <cstdio>

namespace yqlora::log {

void write(Level level, std::string_view message)
{
    const char* prefix = "INFO";
    switch (level) {
    case Level::Info: prefix = "INFO"; break;
    case Level::Warning: prefix = "WARN"; break;
    case Level::Error: prefix = "ERROR"; break;
    }
    std::fprintf(stderr, "[yqlora][%s] %.*s\n", prefix, static_cast<int>(message.size()), message.data());
}

} // namespace yqlora::log
