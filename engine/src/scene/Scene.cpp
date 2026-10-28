#include "yqlora/scene/Scene.hpp"

#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/ExecutionOrder.hpp"

#include <cassert>
#include <utility>

namespace yqlora {

Scene::Scene(const EngineServices& services)
    : m_services(services)
{
}

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

void Scene::enterComponent(Component& component)
{
    m_components[std::type_index(typeid(component))].push_back(&component);
    if (const TypeInfo* type = m_services.registry().find(component.typeName())) {
        const int order = executionOrderOf(m_services, *type);
        for (std::size_t i = 0; i < kLifecyclePhaseCount; ++i) {
            if (type->phases() & phaseBit(static_cast<LifecyclePhase>(i))) {
                m_phases[i].add(component, order);
            }
        }
    }
    m_starting.push_back(&component);
}

void Scene::exitComponent(Component& component)
{
    std::erase(m_components[std::type_index(typeid(component))], &component);
    for (PhaseList& list : m_phases) {
        list.remove(component);
    }
    std::erase(m_starting, &component);
}

void Scene::stop()
{
    if (m_root) {
        m_root->stop();
    }
}

void Scene::advance(float realDelta)
{
    startPending();
    m_time.beginFrame(realDelta);
    while (m_time.takeFixedStep()) {
        runPhase(LifecyclePhase::FixedUpdate, m_time.fixedStep());
    }
    runPhase(LifecyclePhase::Update, m_time.delta());
    runPhase(LifecyclePhase::LateUpdate, m_time.delta());
}

void Scene::startPending()
{
    // Копия: onStart может добавить компоненты, они запустятся в следующем кадре.
    for (Component* component : std::exchange(m_starting, {})) {
        start(*component);
    }
}

void Scene::start(Component& component)
{
    if (!component.m_started) {
        component.m_started = true;
        component.onStart();
    }
}

void Scene::runPhase(LifecyclePhase phase, float seconds)
{
    m_phases[static_cast<std::size_t>(phase)].forEach([&](Component& component) {
        // Добавленный в этом кадре мог ещё не получить onStart.
        start(component);
        switch (phase) {
        case LifecyclePhase::FixedUpdate: component.onFixedUpdate(seconds); break;
        case LifecyclePhase::Update: component.onUpdate(seconds); break;
        case LifecyclePhase::LateUpdate: component.onLateUpdate(seconds); break;
        }
    });
}

} // namespace yqlora
