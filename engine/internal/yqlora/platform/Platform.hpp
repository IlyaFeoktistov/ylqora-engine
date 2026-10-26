#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora::platform {

// Windows: окно консоли для лога. Программа собрана без консоли, поэтому
// по умолчанию её нет. Если программу запустили из терминала, лог идёт в него,
// иначе открывается новое окно. На других системах ничего не делает:
// вывод и так попадает в терминал, из которого запустили программу.
void openConsole();

// Инициализация оконной подсистемы (SDL). Нужна до создания Window.
bool initialize();
void shutdown();

// Папка, где лежит исполняемый файл (рядом с ним лежит assets/).
std::filesystem::path executableDir();

// Папка для пользовательских данных: ~/.local/share/<org>/<app> на Linux,
// %APPDATA%\<org>\<app> на Windows. Создаётся при первом обращении.
std::filesystem::path userDataPath(std::string_view organization, std::string_view application);

// Папка "Документы" пользователя; если система её не знает, домашняя папка.
std::filesystem::path documentsPath();

// Языки пользователя в порядке предпочтения, как их отдаёт ОС: "ru_RU", "en"...
std::vector<std::string> preferredLanguages();

// Время с момента запуска в секундах (монотонное).
double timeSeconds();

} // namespace yqlora::platform
