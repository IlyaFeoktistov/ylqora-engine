#pragma once

#include "yqlora/editor/ui/core/EditorPanel.hpp"
#include "yqlora/editor/ui/panels/ResourceInspector.hpp"

namespace yqlora::editor {

// Свойства выбранного узла и его компоненты. Компоненты можно добавлять,
// менять и удалять, кроме встроенного Transform. Если в "Файлах" выбран
// ресурс, показывает его (ResourceInspector).
class InspectorPanel final : public EditorPanel {
public:
    const char* id() const override
    {
        return "Inspector";
    }

    const char* titleKey() const override
    {
        return "editor.window.inspector";
    }

    void draw(EditorContext& context) override;

private:
    ResourceInspector m_resource;
};

} // namespace yqlora::editor
