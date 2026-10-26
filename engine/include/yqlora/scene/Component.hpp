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
    Node* owner() const { return m_owner; }

    // Встроенные компоненты (Transform) нельзя снять с узла.
    virtual bool removable() const { return true; }

private:
    friend class Node;

    Node* m_owner = nullptr;
};

} // namespace yqlora
