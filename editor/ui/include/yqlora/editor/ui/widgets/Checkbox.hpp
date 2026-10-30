#pragma once

namespace yqlora::editor {

// Флажок меньше поля ввода: padding - отступ вокруг него по вертикали
// (EditorMetrics::checkboxPadding). Стоит посередине строки обычной высоты,
// поэтому подписи рядом не съезжают.
bool checkbox(const char* label, bool* value, float padding);

} // namespace yqlora::editor
