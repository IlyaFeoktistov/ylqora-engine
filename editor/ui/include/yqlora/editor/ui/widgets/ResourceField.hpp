#pragma once

#include "yqlora/editor/ui/widgets/PropertyEditor.hpp"

namespace yqlora::editor {

// Поле ссылки на ресурс, как в Unity: иконка и имя ресурса ("Нет (Меш)",
// если пусто), щелчок по полю или по кнопке справа открывает окно выбора
// (поиск, "Нет", встроенные ресурсы, ассеты проекта; у мешей - по файлам
// моделей, у текстур - с превью). На поле можно отпустить ресурс из
// "Файлов", см. resourceForField. У текстуры под полем - её превью.
// true, если значение поменяли.
bool resourceField(PropertyHint hint, ResourceRef& value, const PropertyEditorContext& context);

} // namespace yqlora::editor
