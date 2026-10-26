#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node.hpp"

#include <concepts>
#include <memory>

namespace yqlora {

class Camera3D;

// Дерево узлов с одним корнем, как сцена в Godot. Новая сцена пуста:
// корня нет, пока его не создадут (любого типа: Node, Node3D, ...).
class Scene {
public:
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

    // Забирает дерево из сцены, сцена становится пустой.
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

    void update(float deltaSeconds);

private:
    std::unique_ptr<Node> m_root;
    Camera3D* m_activeCamera = nullptr;
    Vec4 m_clearColor{0.08f, 0.09f, 0.12f, 1.0f};
};

} // namespace yqlora
