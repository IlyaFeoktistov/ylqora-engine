#pragma once

#include "yqlora/core/Export.hpp"

#include <string>
#include <string_view>

namespace yqlora {

// Перевод текста интерфейса по ключу на текущий язык. Если перевода нет,
// возвращается сам ключ. Берёт локализацию, которую включило приложение:
// это общая служба движка, как и лог, и живёт в библиотеке движка в одном экземпляре.
YQLORA_API std::string tr(std::string_view key);

} // namespace yqlora
