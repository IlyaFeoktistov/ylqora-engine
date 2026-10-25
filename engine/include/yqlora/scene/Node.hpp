#pragma once

#include <concepts>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

// Базовый элемент дерева сцены. Родитель владеет детьми, ребёнок хранит
// невладеющий указатель на родителя. Имена и типы — всегда на английском.
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

    // Обновляет узел, затем всех детей (сверху вниз).
    void update(float deltaSeconds);

protected:
    virtual void onUpdate(float /*deltaSeconds*/) {}
    virtual void onParentChanged() {}

private:
    std::string m_name;
    Node* m_parent = nullptr;
    std::vector<std::unique_ptr<Node>> m_children;
};

} // namespace yqlora
