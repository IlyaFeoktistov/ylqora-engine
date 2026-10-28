#include "yqlora/scene/Scene.hpp"

#include <cassert>

namespace yqlora {

Scene::~Scene()
{
    setRoot(nullptr);
}

void Scene::setRoot(std::unique_ptr<Node> root)
{
    assert((!root || !root->parent()) && "setRoot: node already has a parent");
    // Камера могла жить в старом дереве, указатель на неё сейчас станет висячим.
    m_activeCamera = nullptr;
    if (m_root) {
        m_root->stop();
        m_root->exitScene();
    }
    m_root = std::move(root);
    if (m_root) {
        m_root->enterScene(*this);
    }
}

std::unique_ptr<Node> Scene::takeRoot()
{
    m_activeCamera = nullptr;
    if (m_root) {
        m_root->stop();
        m_root->exitScene();
    }

    return std::move(m_root);
}

std::span<Component* const> Scene::componentsOfType(std::type_index type) const
{
    const auto it = m_components.find(type);
    if (it == m_components.end()) {
        return {};
    }

    return it->second;
}

void Scene::indexComponent(Component& component)
{
    m_components[std::type_index(typeid(component))].push_back(&component);
}

void Scene::unindexComponent(Component& component)
{
    auto& list = m_components[std::type_index(typeid(component))];
    std::erase(list, &component);
}

void Scene::stop()
{
    if (m_root) {
        m_root->stop();
    }
}

void Scene::update(float deltaSeconds)
{
    if (m_root) {
        m_root->update(deltaSeconds);
    }
}

} // namespace yqlora
