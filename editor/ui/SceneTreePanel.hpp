#pragma once

#include "AddNodeDialog.hpp"
#include "core/EditorPanel.hpp"

#include "yqlora/scene/Node.hpp"

namespace yqlora {

class Selection;

// Дерево узлов сцены. Выбор здесь и в окне сцены общий: узел, выбранный
// кликом в 3D, подсвечивается в дереве, а его родители раскрываются.
// Отсюда же узлы добавляются и удаляются.
class SceneTreePanel final : public EditorPanel {
public:
    const char* id() const override
    {
        return "Scene";
    }

    const char* titleKey() const override
    {
        return "editor.window.scene";
    }

    void draw(EditorContext& context) override;

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
