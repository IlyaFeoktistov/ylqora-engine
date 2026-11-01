#include "yqlora/import/TextureImport.hpp"

#include "yqlora/core/Log.hpp"

#include <algorithm>
#include <span>
#include <string>

namespace yqlora {

namespace {

constexpr const char* kColorSpaceKey = "colorSpace";
constexpr const char* kFilterKey = "filter";
constexpr const char* kWrapKey = "wrap";
constexpr const char* kMipmapsKey = "mipmaps";

template <typename E>
void readEnum(const nlohmann::json& import, const char* key, std::span<const NamedValue<E>> names, E& value)
{
    const auto it = import.find(key);
    if (it == import.end()) {
        return;
    }
    const auto found = std::ranges::find_if(names, [&](const NamedValue<E>& named) {
        return it->is_string() && it->template get<std::string>() == named.name;
    });
    if (found == names.end()) {
        log::warn("Texture import setting '{}' has an unknown value {}, using the default", key, it->dump());

        return;
    }
    value = found->value;
}

template <typename E>
std::string_view nameOf(std::span<const NamedValue<E>> names, E value)
{
    return std::ranges::find(names, value, &NamedValue<E>::value)->name;
}

} // namespace

TextureSampling textureSamplingFromJson(const nlohmann::json& import)
{
    TextureSampling sampling;
    if (!import.is_object()) {
        return sampling;
    }
    readEnum<ColorSpace>(import, kColorSpaceKey, kColorSpaceNames, sampling.colorSpace);
    readEnum<TextureFilter>(import, kFilterKey, kTextureFilterNames, sampling.filter);
    readEnum<TextureWrap>(import, kWrapKey, kTextureWrapNames, sampling.wrap);
    if (const auto mipmaps = import.find(kMipmapsKey); mipmaps != import.end() && mipmaps->is_boolean()) {
        sampling.mipmaps = mipmaps->get<bool>();
    }

    return sampling;
}

nlohmann::json toJson(const TextureSampling& sampling)
{
    const TextureSampling defaults;
    nlohmann::json json = nlohmann::json::object();
    if (sampling.colorSpace != defaults.colorSpace) {
        json[kColorSpaceKey] = nameOf<ColorSpace>(kColorSpaceNames, sampling.colorSpace);
    }
    if (sampling.filter != defaults.filter) {
        json[kFilterKey] = nameOf<TextureFilter>(kTextureFilterNames, sampling.filter);
    }
    if (sampling.wrap != defaults.wrap) {
        json[kWrapKey] = nameOf<TextureWrap>(kTextureWrapNames, sampling.wrap);
    }
    if (sampling.mipmaps != defaults.mipmaps) {
        json[kMipmapsKey] = sampling.mipmaps;
    }

    return json;
}

} // namespace yqlora
