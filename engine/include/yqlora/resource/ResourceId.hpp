#pragma once

#include <cstdint>

namespace yqlora {

// Ресурс проекта (меш, материал, текстура...) по ID, а не по пути: файл можно
// переименовать или перенести, и ссылки на него не сломаются. 0 - ресурса нет.
using ResourceId = std::uint64_t;

} // namespace yqlora
