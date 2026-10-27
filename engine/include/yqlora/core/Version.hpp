#pragma once

#include "yqlora/core/Export.hpp"

#include <string_view>

namespace yqlora {

// Версия движка из CMakeLists.txt, например "0.0.1".
YQLORA_API std::string_view engineVersion();

} // namespace yqlora
