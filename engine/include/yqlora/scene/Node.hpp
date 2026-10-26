#pragma once

#include "yqlora/scene/Component.hpp"

#include <concepts>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

// Базовый элемент дерева сцены. Родитель владеет детьми, ребёнок хранит
// невладеющий указатель на родителя. Имена и типы — всегда на английском.
//
// Сам Node не имеет положения — это делают Node2D и Node3D, у которых
// Transform есть всегда. Остальные компоненты подключаются по необходимости.
class Node {
public:
    explicit Node(std::string name = "Node");
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    virtual std::string_view typeName() const { return "Node"; }

    const std::string& name() const { return m_name; }
    void setName(std::string name) { m_name = std::move(name); }

    Node* parent() const { return m_parent; }
    std::span<const std::unique_ptr<Node>> children() const { return m_children; }

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

    // Отцепляет ребёнка и возвращает владение им; nullptr, если это не наш ребёнок.
    std::unique_ptr<Node> removeChild(Node& child);

    // Поиск среди прямых детей по имени.
    Node* findChild(std::string_view name) const;

    std::span<const std::unique_ptr<Component>> components() const { return m_components; }

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

    // Отключает компонент и возвращает владение им; nullptr, если он не наш
    // или его нельзя снять (Transform).
    std::unique_ptr<Component> removeComponent(Component& component);

    // Обновляет узел, затем всех детей (сверху вниз).
    void update(float deltaSeconds);

protected:
    virtual void onUpdate(float /*deltaSeconds*/) {}
    virtual void onParentChanged() {}

private:
    std::string m_name;
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<std::unique_ptr<Node>> m_children;
};

} // namespace yqlora
