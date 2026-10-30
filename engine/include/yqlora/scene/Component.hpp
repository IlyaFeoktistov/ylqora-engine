#pragma once

#include "yqlora/core/Export.hpp"

#include "yqlora/reflection/TypeName.hpp"

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

    // Корень иерархии компонентов, как Node у узлов: typeName чисто
    // виртуальный, поэтому без YQLORA_TYPE. Базы в реестре нет.
    using YqloraThisType = Component;
    using YqloraBaseType = void;

    static constexpr std::string_view staticTypeName()
    {
        return "Component";
    }

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

    // Выключенный компонент не участвует в игре: не получает фаз кадра,
    // рендер и подсистемы его пропускают. Остаётся на узле и в файле сцены.
    // Встроенный компонент всегда включён.
    bool enabled() const
    {
        return m_enabled;
    }

    void setEnabled(bool enabled);

    // Включён и его узел активен в иерархии: компонент участвует в игре.
    bool activeAndEnabled() const;

protected:
    // Жизненный цикл в игре. onStart - перед первым обновлением компонента:
    // при запуске сцены и если компонент добавили во время игры. onStop - игра
    // остановлена или компонент сняли с узла: время освободить взятое в onStart.
    virtual void onStart()
    {
    }

    // Компонент начал или перестал участвовать в игре (см. activeAndEnabled):
    // его включили или выключили, узел или его предок стал активным или
    // неактивным. onEnable приходит перед onStart и перед первой фазой после
    // включения; onDisable - сразу при выключении и перед onStop.
    virtual void onEnable()
    {
    }

    virtual void onDisable()
    {
    }

    // Фазы кадра, см. Scene::advance. Сцена вызывает только те, что
    // переопределены в типе: пустые не стоят ничего.
    virtual void onFixedUpdate(float /*stepSeconds*/)
    {
    }

    virtual void onUpdate(float /*deltaSeconds*/)
    {
    }

    virtual void onLateUpdate(float /*deltaSeconds*/)
    {
    }

    virtual void onStop()
    {
    }

    // Узел с компонентом попал в дерево сцены или ушёл из него (и в редакторе,
    // и в игре). Здесь компонент регистрируется в подсистемах сцены: физике,
    // звуке и так далее. Не путать с onStart: тот только для игры.
    virtual void onEnterScene()
    {
    }

    virtual void onExitScene()
    {
    }

private:
    friend class Node;
    friend class ComponentLifecycle;
    // Для регистрации самого Component: его фазы защищённые.
    friend struct PhaseDetector;

    Node* m_owner = nullptr;
    bool m_builtin = false;
    bool m_enabled = true;
    // Получил onEnable и ещё не получил onDisable.
    bool m_enableCalled = false;
    bool m_started = false;
};

} // namespace yqlora
