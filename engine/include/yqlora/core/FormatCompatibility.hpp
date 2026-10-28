#pragma once

namespace yqlora {

// Можно ли прочитать файл движка (проект, сцену) с такой версией формата.
enum class FormatCompatibility {
    Current,
    // Старый формат, который можно перевести на текущий.
    Older,
    // Сохранён более новой версией движка.
    Newer,
    // Без версии или с версией, перевода с которой нет.
    Unsupported,
};

} // namespace yqlora
