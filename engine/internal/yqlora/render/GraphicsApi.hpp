#pragma once

#include <string_view>

namespace yqlora {

enum class GraphicsApi {
    OpenGL,
    Vulkan,
    Direct3D12,
};

constexpr std::string_view toString(GraphicsApi api)
{
    switch (api) {
    case GraphicsApi::OpenGL: return "OpenGL";
    case GraphicsApi::Vulkan: return "Vulkan";
    case GraphicsApi::Direct3D12: return "Direct3D12";
    }

    return "Unknown";
}

} // namespace yqlora
