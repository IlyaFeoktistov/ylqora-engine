#pragma once

#include "yqlora/render/FrameData.hpp"
#include "yqlora/scene/Node.hpp"

#include <glm/vec4.hpp>

namespace yqlora {

class Camera3D;

class Scene {
public:
    Scene();

    Node& root() { return m_root; }
    const Node& root() const { return m_root; }

    // Камера, через которую смотрим. Должна принадлежать этой сцене;
    // при удалении камеры из дерева сбросьте её через setActiveCamera(nullptr).
    void setActiveCamera(Camera3D* camera) { m_activeCamera = camera; }
    Camera3D* activeCamera() const { return m_activeCamera; }

    void setClearColor(const glm::vec4& color) { m_clearColor = color; }
    const glm::vec4& clearColor() const { return m_clearColor; }

    void update(float deltaSeconds);

    // Снимок сцены для рендерера на текущий кадр.
    FrameData buildFrameData(float aspect, DepthRange depthRange) const;

private:
    Node m_root;
    Camera3D* m_activeCamera = nullptr;
    glm::vec4 m_clearColor{0.08f, 0.09f, 0.12f, 1.0f};
};

} // namespace yqlora
