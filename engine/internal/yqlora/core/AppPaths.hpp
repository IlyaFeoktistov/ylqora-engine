#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>
#include <string_view>

namespace yqlora {

// Название разработчика движка: из него строятся папки в профиле пользователя.
inline constexpr std::string_view kOrganization = "Yqlora";

// Папка данных приложения в профиле пользователя (настройки, списки).
// application - латиница без пробелов.
YQLORA_API std::filesystem::path userDataDirectory(std::string_view application);

// Ресурсы движка рядом с программой: переводы, шрифты, темы.
YQLORA_API std::filesystem::path assetsDirectory();

} // namespace yqlora
