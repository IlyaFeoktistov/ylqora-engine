#pragma once

#include "yqlora/core/Export.hpp"

#include <string_view>

namespace yqlora {

class Node;

// Часть поведения или данных, подключаемая к узлу. Узел владеет своими
// компонентами; компонент хранит невладеющий указатель на узел.
class YQLORA_API Component {
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

protected:
    // Жизненный цикл в игре. onStart - перед первым обновлением компонента:
    // при запуске сцены и если компонент добавили во время игры. onStop - игра
    // остановлена или компонент сняли с узла: время освободить взятое в onStart.
    virtual void onStart()
    {
    }

    virtual void onUpdate(float /*deltaSeconds*/)
    {
    }

    virtual void onStop()
    {
    }

private:
    friend class Node;

    Node* m_owner = nullptr;
    bool m_builtin = false;
    bool m_started = false;
};

} // namespace yqlora
