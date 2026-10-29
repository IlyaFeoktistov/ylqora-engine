#pragma once

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/GameTime.hpp"
#include "yqlora/scene/LifecyclePhase.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/PhaseList.hpp"

#include <array>
#include <concepts>
#include <memory>
#include <ranges>
#include <span>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace yqlora {

class Camera3D;

// Дерево узлов с одним корнем. Новая сцена пуста:
// корня нет, пока его не создадут (любого типа: Node, Node3D, ...).
class YQLORA_API Scene {
public:
    // Службы задаёт владелец сцены и они переживают её; см. EngineServices::none().
    explicit Scene(const EngineServices& services);
    ~Scene();

    // Узлы сцены хранят указатель на неё, поэтому сцену не копируют и не перемещают.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    const EngineServices& services() const
    {
        return m_services;
    }

    bool empty() const
    {
        return !m_root;
    }

    // nullptr, если сцена пуста.
    Node* root()
    {
        return m_root.get();
    }

    const Node* root() const
    {
        return m_root.get();
    }

    // Создаёт корень типа T. Прежний корень со всем деревом уничтожается.
    template <std::derived_from<Node> T, typename... Args>
    T& createRoot(Args&&... args)
    {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *node;
        setRoot(std::move(node));

        return ref;
    }

    // Делает узел корнем (у него не должно быть родителя). Прежний корень
    // уничтожается; nullptr очищает сцену. Активная камера сбрасывается.
    void setRoot(std::unique_ptr<Node> root);

    // Забирает дерево из сцены, сцена становится пустой. Запущенное в игре останавливается.
    std::unique_ptr<Node> takeRoot();

    // Камера, через которую смотрим. Должна принадлежать этой сцене;
    // при удалении камеры из дерева сбросьте её через setActiveCamera(nullptr).
    void setActiveCamera(Camera3D* camera)
    {
        m_activeCamera = camera;
    }

    Camera3D* activeCamera() const
    {
        return m_activeCamera;
    }

    void setClearColor(const Vec4& color)
    {
        m_clearColor = color;
    }

    const Vec4& clearColor() const
    {
        return m_clearColor;
    }

    // Компоненты в дереве сцены ровно этого типа (без наследников), в порядке
    // попадания в сцену. Так подсистемы находят свои компоненты без обхода дерева.
    template <std::derived_from<Component> T>
    auto components() const
    {
        return componentsOfType(typeid(T)) | std::views::transform([](Component* component) {
                   return static_cast<T*>(component);
               });
    }

    std::span<Component* const> componentsOfType(std::type_index type) const;

    GameTime& time()
    {
        return m_time;
    }

    const GameTime& time() const
    {
        return m_time;
    }

    // Кадр игры длиной realDelta секунд по часам. Весь порядок кадра - здесь:
    // onEnable и onStart новым компонентам, фиксированные шаги, update,
    // lateUpdate. Вызывают только участвующие компоненты (activeAndEnabled),
    // у типа которых эта фаза переопределена. Исключение из вызова жизненного
    // цикла выключает компонент и уходит в лог, кадр идёт дальше.
    void advance(float realDelta);

    // Игра остановлена: onDisable и onStop у всего, что успело запуститься.
    void stop();

private:
    friend class Node;
    friend class Component;
    // Компонент вошёл в дерево сцены или вышел из него: индекс по типам
    // и, если он участвует в игре, списки фаз.
    void enterComponent(Component& component);
    void exitComponent(Component& component);

    // Компонент начал участвовать в игре: в списки фаз и в очередь onEnable.
    void activate(Component& component);
    // Перестал: из списков и очереди, onDisable, если onEnable он получил.
    void deactivate(Component& component);
    // onDisable и onStop, если компонент их ждёт.
    void stopComponent(Component& component);

    void startPending();
    // onEnable и onStart, если компонент участвует и ещё их не получил.
    void start(Component& component);
    void runPhase(LifecyclePhase phase, float seconds);

    const EngineServices& m_services;
    GameTime m_time;
    std::array<PhaseList, kLifecyclePhaseCount> m_phases;
    // Участвуют и ещё не получили onEnable или onStart.
    std::vector<Component*> m_starting;
    // Очередь, которую сейчас обходит startPending; выключенный по ходу
    // обхода заменяется на nullptr.
    std::vector<Component*> m_startBatch;
    std::unique_ptr<Node> m_root;
    std::unordered_map<std::type_index, std::vector<Component*>> m_components;
    Camera3D* m_activeCamera = nullptr;
    Vec4 m_clearColor{0.08f, 0.09f, 0.12f, 1.0f};
};

} // namespace yqlora
