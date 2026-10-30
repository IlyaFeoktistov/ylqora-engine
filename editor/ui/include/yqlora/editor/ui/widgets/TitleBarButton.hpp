#pragma once

namespace yqlora::editor {

// Кнопка "развернуть" в заголовке текущего окна, слева от крестика, того же
// вида, что крестик ImGui. У пристыкованного окна - в полосе вкладок его
// узла, и только у выбранной вкладки. Вызывать сразу после Begin.
// true - кнопку нажали.
bool maximizeButton(bool maximized);

} // namespace yqlora::editor
