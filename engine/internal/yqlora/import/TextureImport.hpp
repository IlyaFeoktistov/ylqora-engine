#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/render/TextureData.hpp"

#include <nlohmann/json.hpp>

#include <array>
#include <string_view>

namespace yqlora {

// Имя значения в настройках импорта (.meta) и в редакторе.
template <typename E>
struct NamedValue {
    E value;
    std::string_view name;
};

inline constexpr std::array<NamedValue<ColorSpace>, 2> kColorSpaceNames{{
    {ColorSpace::Srgb, "sRGB"},
    {ColorSpace::Linear, "Linear"},
}};

inline constexpr std::array<NamedValue<TextureFilter>, 2> kTextureFilterNames{{
    {TextureFilter::Linear, "Linear"},
    {TextureFilter::Nearest, "Nearest"},
}};

inline constexpr std::array<NamedValue<TextureWrap>, 3> kTextureWrapNames{{
    {TextureWrap::Repeat, "Repeat"},
    {TextureWrap::Clamp, "Clamp"},
    {TextureWrap::Mirror, "Mirror"},
}};

// Настройки импорта текстуры из объекта JSON .meta. Ключа нет - значение по
// умолчанию; неизвестное значение - тоже, с предупреждением в логе.
YQLORA_API TextureSampling textureSamplingFromJson(const nlohmann::json& import);

// Объект JSON для .meta; значения по умолчанию не пишутся.
YQLORA_API nlohmann::json toJson(const TextureSampling& sampling);

} // namespace yqlora
