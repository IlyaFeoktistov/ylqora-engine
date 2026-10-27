#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <optional>
#include <system_error>

namespace yqlora {

// Чтение и запись JSON-файлов движка и редактора (настройки, проекты, сцены,
// .meta): одна обработка ошибок и один вид файла. Только заголовок: типы
// nlohmann наружу из библиотеки движка не выходят.

inline constexpr int kJsonIndent = 4;

// nullopt, если файла нет или в нём не JSON.
inline std::optional<nlohmann::json> readJsonFile(const std::filesystem::path& file)
{
    std::ifstream in(file);
    if (!in) {
        return std::nullopt;
    }
    nlohmann::json json = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
    if (json.is_discarded()) {
        return std::nullopt;
    }

    return json;
}

// Пишет через временный файл и переименование: при сбое на середине старый
// файл остаётся целым. Папку создаёт, если её нет.
inline bool writeJsonFile(const std::filesystem::path& file, const nlohmann::json& json)
{
    std::error_code ec;
    if (file.has_parent_path()) {
        std::filesystem::create_directories(file.parent_path(), ec);
    }
    std::filesystem::path temporary = file;
    temporary += ".tmp";
    {
        std::ofstream out(temporary, std::ios::trunc);
        if (!out) {
            return false;
        }
        out << json.dump(kJsonIndent) << '\n';
        if (!out) {
            return false;
        }
    }
    std::filesystem::rename(temporary, file, ec);

    return !ec;
}

} // namespace yqlora
