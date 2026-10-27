#pragma once

#include <cstdint>

namespace yqlora {

// Уникален и не меняется за жизнь узла, 0 означает, что узла нет.
using NodeId = std::uint64_t;

} // namespace yqlora
