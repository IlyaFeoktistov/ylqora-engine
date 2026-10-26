#pragma once

#include "yqlora/core/Log.hpp"

#include <filesystem>

namespace yqlora::log {

// Кроме консоли, лог пишется в этот файл. Файл перезаписывается при каждом
// запуске: без консоли (обычный запуск на Windows) это единственный след работы.
void openFile(const std::filesystem::path& file);

// Сообщения ниже этого уровня не пишутся и даже не форматируются.
// По умолчанию Warning: в обычном запуске остаются только предупреждения
// и ошибки, всё остальное включает флаг --debug.
void setMinimumLevel(Level level);

} // namespace yqlora::log
