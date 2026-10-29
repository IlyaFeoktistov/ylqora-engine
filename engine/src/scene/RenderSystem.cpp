#include "yqlora/scene/RenderSystem.hpp"

#include "yqlora/scene/Camera3D.hpp"

#include <algorithm>

namespace yqlora {

std::vector<const Camera3D*> RenderSystem::cameras() const
{
    std::vector<const Camera3D*> result;
    std::ranges::copy_if(m_cameras, std::back_inserter(result), [](const Camera3D* camera) {
        return camera->activeInHierarchy();
    });
    std::ranges::stable_sort(result, std::less<>{}, &Camera3D::order);

    return result;
}

const Camera3D* RenderSystem::mainCamera() const
{
    const auto active = cameras();

    return active.empty() ? nullptr : active.front();
}

void RenderSystem::addCamera(const Camera3D& camera)
{
    m_cameras.push_back(&camera);
}

void RenderSystem::removeCamera(const Camera3D& camera)
{
    std::erase(m_cameras, &camera);
}

} // namespace yqlora
