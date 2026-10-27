#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>
#include <string>

namespace yqlora {

// Пользовательские настройки, которые переживают перезапуск программы.
// Хранятся в JSON в папке пользователя (см. platform::userDataPath()).
class YQLORA_API Settings {
public:
    explicit Settings(std::filesystem::path file);

    // false, если файла нет или он повреждён; поля остаются значениями по умолчанию.
    bool load();

    // Сохраняет известные поля, не трогая чужие ключи в файле
    // (чтобы будущие версии движка не теряли настройки друг друга).
    bool save() const;

    const std::filesystem::path& path() const
    {
        return m_path;
    }

    // Код языка интерфейса ("ru", "en"). Пусто, если язык ещё не выбран.
    std::string language;

private:
    std::filesystem::path m_path;
};

} // namespace yqlora
