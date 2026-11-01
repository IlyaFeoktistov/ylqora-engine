#pragma once

#include "yqlora/resource/ResourceLoader.hpp"

namespace yqlora {

// Картинки (PNG, JPG, TGA, BMP) в TextureData, всегда RGBA. Настройки
// импорта - TextureSampling (textureSamplingFromJson).
class YQLORA_API ImageLoader final : public ResourceLoader {
public:
    std::span<const std::string_view> extensions() const override;
    std::string_view kind() const override;
    LoadResult load(const LoadRequest& request) const override;
};

} // namespace yqlora
