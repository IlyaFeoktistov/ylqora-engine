#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/core/FormatCompatibility.hpp"

#include <nlohmann/json.hpp>

#include <span>
#include <string_view>

namespace yqlora {

// Перевод документа с одной версии формата на следующую.
using Migration = void (*)(nlohmann::json& document);

// Версии формата одного вида файлов (сцена, проект) и переводы между ними.
// Текущая версия выводится из числа переводов: добавил перевод - версия выросла.
// Ключи старой версии перевод объявляет у себя, а не берёт текущие: если
// текущий формат что-то переименует, перевод со старой версии не изменится.
class YQLORA_API FileFormat {
public:
    // migrations[i] переводит версию first + i в first + i + 1.
    constexpr FileFormat(std::string_view name, int first, std::span<const Migration> migrations)
        : m_name(name)
        , m_first(first)
        , m_migrations(migrations)
    {
    }

    int current() const
    {
        return m_first + static_cast<int>(m_migrations.size());
    }

    FormatCompatibility compatibility(int version) const;

    // Версия из документа; 0, если её нет.
    static int versionOf(const nlohmann::json& document);

    // Записывает в документ текущую версию.
    void stamp(nlohmann::json& document) const;

    // Приводит документ к текущей версии. false с ошибкой в логе, если версии
    // нет, она новее текущей или перевода с неё нет.
    bool migrate(nlohmann::json& document) const;

private:
    std::string_view m_name;
    int m_first;
    std::span<const Migration> m_migrations;
};

} // namespace yqlora
