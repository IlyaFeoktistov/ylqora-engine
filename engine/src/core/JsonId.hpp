#pragma once

#include <nlohmann/json.hpp>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>

namespace yqlora {

// 64-битные ID (узлов, ресурсов) в JSON пишутся строкой из 16 шестнадцатеричных
// цифр: многие инструменты (JavaScript и всё, что на нём) читают числа только
// до 2^53 и молча портили бы ID. Только для файлов движка, наружу не виден.
inline nlohmann::json idToJson(std::uint64_t id)
{
    char text[17] = {};
    const auto [end, error] = std::to_chars(text, text + 16, id, 16);
    const std::string digits(text, end);

    return std::string(16 - digits.size(), '0') + digits;
}

// Читает и строку, и число: так пишут файлы первых версий форматов.
inline std::optional<std::uint64_t> idFromJson(const nlohmann::json& json)
{
    if (json.is_number_unsigned()) {
        return json.get<std::uint64_t>();
    }
    if (!json.is_string()) {
        return std::nullopt;
    }
    const std::string& text = json.get_ref<const std::string&>();
    std::uint64_t id = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), id, 16);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty()) {
        return std::nullopt;
    }

    return id;
}

} // namespace yqlora
