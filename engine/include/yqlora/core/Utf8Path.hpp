#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace yqlora {

// Путь как текст в UTF-8: для логов, интерфейса и библиотек, которые ждут
// UTF-8 (ImGui, SDL). path.string() не годится: на Windows он переводит путь
// в кодовую страницу системы, и кириллица ломается. Разделители родные для
// системы, как их привык видеть пользователь.
inline std::string toUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.u8string();

    return {text.begin(), text.end()};
}

// То же, но разделители всегда прямые слэши: для путей, которые пишутся в
// файлы или показываются как путь внутри проекта, одинаково на всех системах.
inline std::string toGenericUtf8(const std::filesystem::path& path)
{
    const std::u8string text = path.generic_u8string();

    return {text.begin(), text.end()};
}

// Обратно: текст в UTF-8 (из поля ввода, из файла) в путь.
inline std::filesystem::path fromUtf8(std::string_view text)
{
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

} // namespace yqlora
