#include "yqlora/scene/Scene.hpp"

#include <cassert>

namespace yqlora {

void Scene::setRoot(std::unique_ptr<Node> root)
{
    assert((!root || !root->parent()) && "setRoot: node already has a parent");
    // Камера могла жить в старом дереве, указатель на неё сейчас станет висячим.
    m_activeCamera = nullptr;
    m_root = std::move(root);
}

std::unique_ptr<Node> Scene::takeRoot()
{
    m_activeCamera = nullptr;

    return std::move(m_root);
}

void Scene::update(float deltaSeconds)
{
    if (m_root) {
        m_root->update(deltaSeconds);
    }
}

} // namespace yqlora
