#pragma once

#include "yqlora/editor/ui/core/EditorPanel.hpp"

namespace yqlora::editor {

// Свойства выбранного узла и его компоненты. Компоненты можно добавлять,
// менять и удалять, кроме встроенного Transform.
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
};

} // namespace yqlora::editor
