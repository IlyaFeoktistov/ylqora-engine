#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node.hpp"

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
    Scene() = default;
    ~Scene();

    // Узлы сцены хранят указатель на неё, поэтому сцену не копируют и не перемещают.
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

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

    // Кадр игры для всего дерева. См. Node::update.
    void update(float deltaSeconds);

    // Игра остановлена: onStop у всего, что успело запуститься.
    void stop();

private:
    friend class Node;
    void indexComponent(Component& component);
    void unindexComponent(Component& component);

    std::unique_ptr<Node> m_root;
    std::unordered_map<std::type_index, std::vector<Component*>> m_components;
    Camera3D* m_activeCamera = nullptr;
    Vec4 m_clearColor{0.08f, 0.09f, 0.12f, 1.0f};
};

} // namespace yqlora
