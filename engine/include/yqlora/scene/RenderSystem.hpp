#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/SceneSystem.hpp"

#include <vector>

namespace yqlora {

class Camera3D;

// Камеры сцены, через которые её видно. Рисуются по очереди, каждая со своей
// очисткой и маской слоёв; снимок кадра для рендерера собирает Extract
// (extractFrame во внутреннем API рендера).
class YQLORA_API RenderSystem final : public SceneSystem {
public:
    YQLORA_TYPE(RenderSystem, SceneSystem)

    using SceneSystem::SceneSystem;

    // Камеры с активным узлом в порядке отрисовки: меньший order раньше,
    // при равном - кто раньше вошёл в сцену.
    std::vector<const Camera3D*> cameras() const;

    // Главная камера - первая в порядке отрисовки, через неё видно игру.
    // nullptr, если активных камер нет.
    const Camera3D* mainCamera() const;

private:
    // Камера сама входит в список, когда входит в сцену, и уходит вместе с ней.
    friend class Camera3D;
    void addCamera(const Camera3D& camera);
    void removeCamera(const Camera3D& camera);

    std::vector<const Camera3D*> m_cameras;
};

} // namespace yqlora
