#pragma once

#include <string_view>

namespace yqlora {

class Node;

// Часть поведения или данных, подключаемая к узлу. Узел владеет своими
// компонентами; компонент хранит невладеющий указатель на узел.
class Component {
public:
    Component() = default;
    virtual ~Component();

    Component(const Component&) = delete;
    Component& operator=(const Component&) = delete;

    virtual std::string_view typeName() const = 0;

    // Узел, к которому подключён компонент; nullptr, пока не подключён.
    Node* owner() const
    {
        return m_owner;
    }

    // Встроенный компонент (Transform) создаётся вместе с узлом, и снять его нельзя.
    // Флаг ставит только узел, сам компонент объявить себя встроенным не может.
    bool builtin() const
    {
        return m_builtin;
    }

private:
    friend class Node;

    Node* m_owner = nullptr;
    bool m_builtin = false;
};

} // namespace yqlora
