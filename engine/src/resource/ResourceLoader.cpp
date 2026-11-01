#include "yqlora/resource/ResourceLoader.hpp"

namespace yqlora {

std::string SubresourceKeys::next(std::string_view kind, std::string_view name, std::size_t indexInFile)
{
    const std::string base = std::string(kind) + ":" + std::string(name);
    std::string key = name.empty() ? base + "#" + std::to_string(indexInFile) : base;
    for (std::size_t copy = 1; m_used.contains(key); ++copy) {
        key = base + "#" + std::to_string(copy);
    }
    m_used.insert(key);

    return key;
}

} // namespace yqlora
