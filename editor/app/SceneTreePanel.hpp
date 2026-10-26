#pragma once

#include "yqlora/scene/Node.hpp"

namespace yqlora {

class Scene;
class Selection;

// Дерево узлов сцены. Выбор здесь и в окне сцены общий: узел, выбранный
// кликом в 3D, подсвечивается в дереве, а его родители раскрываются.
class SceneTreePanel {
public:
    void draw(Scene& scene, Selection& selection);

private:
    void drawNode(const Node& node, Selection& selection);

    // Что было выбрано в прошлом кадре: если выбор сменился не здесь,
    // дерево раскрывается и прокручивается к выбранному узлу.
    NodeId m_lastSelected = 0;
    bool m_reveal = false;
};

} // namespace yqlora
