#pragma once

#include "yqlora/scene/Node.hpp"

#include <string>

namespace yqlora {

class TypeInfo;
class TypeRegistry;

} // namespace yqlora

namespace yqlora::editor {

class EditorSession;

// Выбор типа нового узла: дерево типов по наследованию, поиск по имени
// и описание выбранного типа.
class AddNodeDialog {
public:
    // Окно откроется в следующем кадре. parent - куда добавить узел, 0 - в корень.
    void open(NodeId parent);
    void draw(EditorSession& session, const TypeRegistry& registry);

private:
    // Возвращает true, если тип выбрали двойным кликом.
    bool drawType(const TypeRegistry& registry, const TypeInfo& type);
    bool matchesSearch(const TypeInfo& type) const;

    bool m_requested = false;
    NodeId m_parent = 0;
    std::string m_search;
    std::string m_selectedType = "Node3D";
};

} // namespace yqlora::editor
