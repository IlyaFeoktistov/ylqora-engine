#pragma once

#include <string>
#include <string_view>

namespace yqlora {

// Перевод текста интерфейса по ключу на текущий язык. Если перевода нет,
// возвращается сам ключ.
std::string tr(std::string_view key);

} // namespace yqlora
