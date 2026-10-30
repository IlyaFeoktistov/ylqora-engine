#pragma once

namespace yqlora::editor {

// При первом запуске раскладывает окна: дерево слева, инспектор справа,
// файлы снизу, сцена в центре. Если раскладка уже есть (её помнит imgui.ini),
// ничего не меняет.
void buildDefaultLayout(unsigned int dockspace);

} // namespace yqlora::editor
