#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/NodeId.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

// Базовый элемент дерева сцены. Родитель владеет детьми, ребёнок хранит
// невладеющий указатель на родителя. Имена и типы всегда на английском.
//
// Сам Node не имеет положения, это делают Node2D и Node3D, у которых
// Transform есть всегда. Остальные компоненты подключаются по необходимости.
class YQLORA_API Node {
public:
    explicit Node(std::string name = "Node");
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // Корень иерархии: свой typeName без override, поэтому без YQLORA_TYPE.
    using YqloraThisType = Node;

    static constexpr std::string_view staticTypeName()
    {
        return "Node";
    }

    virtual std::string_view typeName() const
    {
        return staticTypeName();
    }

    // Уникален и не меняется за жизнь узла: по нему узлы ссылаются друг
    // на друга в файлах сцен и в редакторе. Выдаётся случайно при создании.
    NodeId id() const
    {
        return m_id;
    }

    const std::string& name() const
    {
        return m_name;
    }

    void setName(std::string name)
    {
        m_name = std::move(name);
    }

    Node* parent() const
    {
        return m_parent;
    }

    std::span<const std::unique_ptr<Node>> children() const
    {
        return m_children;
    }

    // Создаёт узел типа T и делает его последним ребёнком.
    template <std::derived_from<Node> T, typename... Args>
    T& addChild(Args&&... args)
    {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *node;
        addChild(std::move(node));

        return ref;
    }

    Node& addChild(std::unique_ptr<Node> child);

    // Вставляет ребёнка на место index среди детей (не больше их числа),
    // например вернуть удалённый узел туда, где он был.
    Node& insertChild(std::size_t index, std::unique_ptr<Node> child);

    // Отцепляет ребёнка и возвращает владение им; nullptr, если это не наш ребёнок.
    std::unique_ptr<Node> removeChild(Node& child);

    // Поиск среди прямых детей по имени.
    Node* findChild(std::string_view name) const;

    std::span<const std::unique_ptr<Component>> components() const
    {
        return m_components;
    }

    // Создаёт компонент типа T и подключает к узлу.
    template <std::derived_from<Component> T, typename... Args>
    T& addComponent(Args&&... args)
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *component;
        addComponent(std::move(component));

        return ref;
    }

    Component& addComponent(std::unique_ptr<Component> component);

    // Вставляет компонент на место index в списке (не больше его длины).
    Component& insertComponent(std::size_t index, std::unique_ptr<Component> component);

    // Первый компонент типа T (или его наследника); nullptr, если такого нет.
    template <std::derived_from<Component> T>
    T* getComponent() const
    {
        for (const auto& c : m_components) {
            if (auto* typed = dynamic_cast<T*>(c.get())) {
                return typed;
            }
        }

        return nullptr;
    }

    // Компонент наш и не встроенный.
    bool canRemoveComponent(const Component& component) const;

    // Отключает компонент и возвращает владение им; nullptr, если снять нельзя
    // (см. canRemoveComponent).
    std::unique_ptr<Component> removeComponent(Component& component);

    // Кадр игры: узел, его компоненты, затем дети (сверху вниз). Кто
    // обновляется впервые, сначала получает onStart.
    void update(float deltaSeconds);

    // Игра остановлена: onStop у узла, компонентов и детей, которые успели
    // запуститься. Следующий update снова начнёт с onStart.
    void stop();

    bool started() const
    {
        return m_started;
    }

protected:
    // Для Node2D/Node3D: подключает компонент, который нельзя снять.
    Component& addBuiltinComponent(std::unique_ptr<Component> component);

    // Жизненный цикл узла в игре, как у компонентов.
    virtual void onStart()
    {
    }

    virtual void onUpdate(float /*deltaSeconds*/)
    {
    }

    virtual void onStop()
    {
    }

    virtual void onParentChanged()
    {
    }

private:
    // ID из файла сцены выставляет только загрузчик.
    friend class SceneSerializer;
    void setId(NodeId id);

    NodeId m_id;
    std::string m_name;
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<std::unique_ptr<Node>> m_children;
    bool m_started = false;
};

} // namespace yqlora
