#pragma once

#include "AddNodeDialog.hpp"

#include "yqlora/scene/Node.hpp"

namespace yqlora {

class EditorSession;
class Selection;
class TypeRegistry;

// Дерево узлов сцены. Выбор здесь и в окне сцены общий: узел, выбранный
// кликом в 3D, подсвечивается в дереве, а его родители раскрываются.
// Отсюда же узлы добавляются и удаляются.
class SceneTreePanel {
public:
    void draw(EditorSession& session, const TypeRegistry& registry);

private:
    void drawNode(const Node& node, Selection& selection);

    AddNodeDialog m_addNode;
    // Что было выбрано в прошлом кадре: если выбор сменился не здесь,
    // дерево раскрывается и прокручивается к выбранному узлу.
    NodeId m_lastSelected = 0;
    bool m_reveal = false;
    // Удалять узел можно только после обхода дерева.
    NodeId m_deleteRequest = 0;
};

} // namespace yqlora
