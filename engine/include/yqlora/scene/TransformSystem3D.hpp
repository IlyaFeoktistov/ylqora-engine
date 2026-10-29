#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/SceneSystem.hpp"

#include <vector>

namespace yqlora {

class Node3D;

// Мировые матрицы узлов Node3D одним проходом по изменённым: после
// симуляции все матрицы свежие, и Extract читает готовые значения.
// Узлы сами сообщают, что устарели (Node3D::markWorldDirty).
class YQLORA_API TransformSystem3D final : public SceneSystem {
public:
    YQLORA_TYPE(TransformSystem3D, SceneSystem)

    using SceneSystem::SceneSystem;

    void updateTransforms() override;

    // Ждут пересчёта: верхние устаревшие узлы, их ветки устарели вместе с ними.
    std::size_t pendingCount() const
    {
        return m_dirty.size();
    }

private:
    friend class Node3D;

    void markDirty(Node3D& node);
    void forget(Node3D& node);

    // Свежий узел пропускаем: его устаревшие потомки стоят в списке сами.
    static void refreshBranch(Node3D& node);

    std::vector<Node3D*> m_dirty;
};

} // namespace yqlora
