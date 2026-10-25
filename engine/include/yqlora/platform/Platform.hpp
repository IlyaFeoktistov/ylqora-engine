#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora::platform {

// Инициализация оконной подсистемы (SDL). Нужна до создания Window.
bool initialize();
void shutdown();

// Папка, где лежит исполняемый файл (рядом с ним — assets/).
std::filesystem::path executableDir();

// Папка для пользовательских данных: ~/.local/share/<org>/<app> на Linux,
// %APPDATA%\<org>\<app> на Windows. Создаётся при первом обращении.
std::filesystem::path userDataPath(std::string_view organization, std::string_view application);

// Языки пользователя в порядке предпочтения, как их отдаёт ОС: "ru_RU", "en"...
std::vector<std::string> preferredLanguages();

// Время с момента запуска в секундах (монотонное).
double timeSeconds();

} // namespace yqlora::platform
