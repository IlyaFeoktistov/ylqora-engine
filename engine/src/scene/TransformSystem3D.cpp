#include "yqlora/scene/TransformSystem3D.hpp"

#include "yqlora/scene/Node3D.hpp"

#include <algorithm>
#include <utility>

namespace yqlora {

void TransformSystem3D::refreshBranch(Node3D& node)
{
    if (!node.m_worldDirty) {
        return;
    }
    node.worldMatrix();
    for (const auto& child : node.children()) {
        if (auto* child3D = dynamic_cast<Node3D*>(child.get())) {
            refreshBranch(*child3D);
        }
    }
}

void TransformSystem3D::updateTransforms()
{
    for (Node3D* node : std::exchange(m_dirty, {})) {
        refreshBranch(*node);
    }
}

void TransformSystem3D::markDirty(Node3D& node)
{
    m_dirty.push_back(&node);
}

void TransformSystem3D::forget(Node3D& node)
{
    std::erase(m_dirty, &node);
}

} // namespace yqlora
