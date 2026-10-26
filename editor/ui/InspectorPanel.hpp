#pragma once

namespace yqlora {

class EditorSession;
class TypeRegistry;

// Свойства выбранного узла и его компоненты. Компоненты можно добавлять,
// менять и удалять, кроме встроенного Transform.
class InspectorPanel {
public:
    void draw(EditorSession& session, const TypeRegistry& registry);

    // В этом кадре тянут значение мышью: нужен перенос курсора у края окна.
    bool draggingValue() const
    {
        return m_dragging;
    }

private:
    bool m_dragging = false;
};

} // namespace yqlora
