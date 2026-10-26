#pragma once

namespace yqlora {

class EditorSession;
class TypeRegistry;

// Свойства выбранного узла и его компоненты. Компоненты можно добавлять,
// менять и удалять, кроме встроенного Transform.
class InspectorPanel {
public:
    void draw(EditorSession& session, const TypeRegistry& registry);
};

} // namespace yqlora
