#pragma once

#include <filesystem>

namespace yqlora::log {

// Кроме консоли, лог пишется в этот файл. Файл перезаписывается при каждом
// запуске: без консоли (обычный запуск на Windows) это единственный след работы.
void openFile(const std::filesystem::path& file);

} // namespace yqlora::log
