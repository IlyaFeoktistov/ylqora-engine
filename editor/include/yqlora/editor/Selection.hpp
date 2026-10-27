#pragma once

#include "yqlora/scene/NodeId.hpp"

#include <algorithm>
#include <span>
#include <vector>

namespace yqlora::editor {

// Что выбрано в редакторе. Общее для 3D-окна и дерева узлов: выбор в одном
// месте сразу виден в другом. Можно выбрать несколько узлов; последний
// выбранный - основной: его показывает инспектор, на нём стоит манипулятор.
// Хранит ID, а не указатели, чтобы удалённый узел не оставил висячих ссылок.
class Selection {
public:
    // Основной выбранный узел, 0 - ничего не выбрано.
    NodeId selected() const
    {
        return m_nodes.empty() ? 0 : m_nodes.back();
    }

    std::span<const NodeId> nodes() const
    {
        return m_nodes;
    }

    bool empty() const
    {
        return m_nodes.empty();
    }

    bool isSelected(NodeId node) const
    {
        return node != 0 && std::ranges::find(m_nodes, node) != m_nodes.end();
    }

    // Выбирает только этот узел; 0 снимает выбор.
    void select(NodeId node)
    {
        m_nodes.clear();
        if (node != 0) {
            m_nodes.push_back(node);
        }
    }

    // Добавляет узел к выбору или убирает из него (Ctrl+клик). Добавленный становится основным.
    void toggle(NodeId node)
    {
        if (isSelected(node)) {
            deselect(node);
        } else if (node != 0) {
            m_nodes.push_back(node);
        }
    }

    void deselect(NodeId node)
    {
        std::erase(m_nodes, node);
    }

    void clear()
    {
        m_nodes.clear();
    }

    bool operator==(const Selection&) const = default;

private:
    std::vector<NodeId> m_nodes;
};

} // namespace yqlora::editor
