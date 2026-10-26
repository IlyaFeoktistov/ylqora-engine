#pragma once

#include "yqlora/scene/Node.hpp"

namespace yqlora {

// Что выбрано в редакторе. Общее для 3D-окна и дерева узлов: выбор в одном
// месте сразу виден в другом. Хранит ID, а не указатель, чтобы удалённый
// узел не оставил висячую ссылку.
class Selection {
public:
    // 0, если ничего не выбрано.
    NodeId selected() const
    {
        return m_selected;
    }

    bool isSelected(NodeId node) const
    {
        return node != 0 && node == m_selected;
    }

    void select(NodeId node)
    {
        m_selected = node;
    }

    void clear()
    {
        m_selected = 0;
    }

private:
    NodeId m_selected = 0;
};

} // namespace yqlora
