#pragma once

#include <nlohmann/json.hpp>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>

namespace yqlora {

inline constexpr int kHexBase = 16;
// 64 бита - 16 шестнадцатеричных цифр.
inline constexpr std::size_t kIdDigits = 16;

// 64-битные ID (узлов, ресурсов) в JSON пишутся строкой из 16 шестнадцатеричных
// цифр: многие инструменты (JavaScript и всё, что на нём) читают числа только
// до 2^53 и молча портили бы ID. Только для файлов движка, наружу не виден.
inline nlohmann::json idToJson(std::uint64_t id)
{
    char text[kIdDigits] = {};
    const auto [end, error] = std::to_chars(text, text + kIdDigits, id, kHexBase);
    const std::string digits(text, end);

    return std::string(kIdDigits - digits.size(), '0') + digits;
}

inline std::optional<std::uint64_t> idFromJson(const nlohmann::json& json)
{
    if (!json.is_string()) {
        return std::nullopt;
    }
    const std::string& text = json.get_ref<const std::string&>();
    std::uint64_t id = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), id, kHexBase);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty()) {
        return std::nullopt;
    }

    return id;
}

} // namespace yqlora
