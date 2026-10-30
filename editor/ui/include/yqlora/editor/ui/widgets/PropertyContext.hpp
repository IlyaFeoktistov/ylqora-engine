#pragma once

#include "yqlora/editor/ui/core/EditorContext.hpp"
#include "yqlora/editor/ui/widgets/PropertyEditor.hpp"

namespace yqlora::editor {

// Что полям свойств нужно от редактора в этом кадре: тема, сцена, ресурсы и
// имена слоёв открытого проекта, переход к разделу Layers настроек проекта.
PropertyEditorContext propertyEditorContext(const EditorContext& context);

} // namespace yqlora::editor
