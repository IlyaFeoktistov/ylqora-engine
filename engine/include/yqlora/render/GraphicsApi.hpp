#pragma once

#include <array>
#include <cstddef>
#include <string_view>

namespace yqlora {

enum class GraphicsApi {
    OpenGL,
    Vulkan,
    Direct3D12,
};

// Имена значений по порядку GraphicsApi: в логе, в файле настроек и в редакторе.
inline constexpr std::array<std::string_view, 3> kGraphicsApiNames{"OpenGL", "Vulkan", "Direct3D12"};

constexpr std::string_view graphicsApiName(GraphicsApi api)
{
    const auto index = static_cast<std::size_t>(api);

    return index < kGraphicsApiNames.size() ? kGraphicsApiNames[index] : "Unknown";
}

} // namespace yqlora
