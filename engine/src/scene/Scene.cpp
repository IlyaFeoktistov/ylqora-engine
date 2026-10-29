#include "yqlora/scene/Scene.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/ExecutionOrder.hpp"
#include "yqlora/scene/NodeTree.hpp"

#include <algorithm>
#include <cassert>
#include <exception>
#include <string_view>
#include <utility>

namespace yqlora {

namespace {

constexpr std::string_view kUnknownError = "unknown exception";

void disableAfterError(Component& component, std::string_view callName, std::string_view error)
{
    const Node* node = component.owner();
    log::error(
        "{} of component '{}' on node '{}' threw: {}. The component is disabled",
        callName,
        component.typeName(),
        node ? node->name() : std::string(),
        error
    );
    component.setEnabled(false);
}

// Ошибка в коде игры не роняет игру: компонент выключается, ошибка уходит в лог.
template <typename Call>
void guarded(Component& component, std::string_view callName, Call&& call)
{
    try {
        call();
    } catch (const std::exception& error) {
        disableAfterError(component, callName, error.what());
    } catch (...) {
        disableAfterError(component, callName, kUnknownError);
    }
}

constexpr std::string_view phaseCallName(LifecyclePhase phase)
{
    switch (phase) {
    case LifecyclePhase::FixedUpdate: return "onFixedUpdate";
    case LifecyclePhase::Update: return "onUpdate";
    case LifecyclePhase::LateUpdate: return "onLateUpdate";
    }

    return {};
}

} // namespace

Scene::Scene(const EngineServices& services)
    : m_services(services)
{
    for (const auto& type : m_services.registry().types()) {
        if (type->kind() != TypeKind::System) {
            continue;
        }
        if (std::unique_ptr<SceneSystem> system = type->createSystem(*this)) {
            m_systemsByType.emplace(std::type_index(typeid(*system)), system.get());
            m_systems.push_back(std::move(system));
        }
    }
}

Scene::~Scene()
{
    setRoot(nullptr);
}

void Scene::setRoot(std::unique_ptr<Node> root)
{
    assert((!root || !root->parent()) && "setRoot: node already has a parent");
    assert(!m_inFrame && "setRoot: during a frame use Scene::destroy");
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
    assert(!m_inFrame && "takeRoot: during a frame use Scene::destroy");
    if (m_root) {
        m_root->stop();
        m_root->exitScene();
    }

    return std::move(m_root);
}

void Scene::destroy(Node& node)
{
    assert(node.scene() == this && "destroy: the node is not in this scene");
    if (m_inFrame) {
        m_treeChanges.push_back({.node = node.id()});

        return;
    }
    destroyNow(node);
}

void Scene::setParent(Node& node, Node& parent, KeepWorld keepWorld)
{
    assert(node.scene() == this && parent.scene() == this && "setParent: the nodes are not in this scene");
    if (!node.parent() || isInBranch(node, parent)) {
        log::error("Scene: node '{}' cannot be moved under '{}'", node.name(), parent.name());
        assert(false && "setParent: the root cannot be moved, nor a node into its own branch");

        return;
    }
    if (m_inFrame) {
        m_treeChanges.push_back({.node = node.id(), .parent = parent.id(), .keepWorld = keepWorld});

        return;
    }
    node.moveTo(parent, keepWorld);
}

void Scene::applyTreeChange(const TreeChange& change)
{
    Node* node = m_root ? findNode(*m_root, change.node) : nullptr;
    if (!node) {
        // Ушёл раньше вместе с уничтоженной веткой.
        return;
    }
    if (change.parent == 0) {
        destroyNow(*node);

        return;
    }
    Node* parent = findNode(*m_root, change.parent);
    // Прошлые изменения кадра могли сделать перенос невозможным.
    if (!parent || !node->parent() || isInBranch(*node, *parent)) {
        log::error("Scene: node '{}' cannot be moved, the tree has changed during the frame", node->name());

        return;
    }
    node->moveTo(*parent, change.keepWorld);
}

void Scene::destroyNow(Node& node)
{
    if (&node == m_root.get()) {
        setRoot(nullptr);

        return;
    }
    node.parent()->removeChild(node);
}

std::span<Component* const> Scene::componentsOfType(std::string_view typeName) const
{
    const auto it = m_components.find(typeName);
    if (it == m_components.end()) {
        return {};
    }

    return it->second;
}

void Scene::enterComponent(Component& component)
{
    m_components[std::string(component.typeName())].push_back(&component);
    if (component.activeAndEnabled()) {
        activate(component);
    }
}

void Scene::exitComponent(Component& component)
{
    deactivate(component);
    if (const auto it = m_components.find(component.typeName()); it != m_components.end()) {
        std::erase(it->second, &component);
    }
}

void Scene::activate(Component& component)
{
    if (m_inFrame) {
        m_joining.push_back(&component);

        return;
    }
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

void Scene::deactivate(Component& component)
{
    for (PhaseList& list : m_phases) {
        list.remove(component);
    }
    std::erase(m_starting, &component);
    std::erase(m_joining, &component);
    std::ranges::replace(m_startBatch, &component, nullptr);
    if (component.m_enableCalled) {
        component.m_enableCalled = false;
        guarded(component, "onDisable", [&] {
            component.onDisable();
        });
    }
}

void Scene::stopComponent(Component& component)
{
    if (component.m_enableCalled) {
        component.m_enableCalled = false;
        guarded(component, "onDisable", [&] {
            component.onDisable();
        });
    }
    if (component.m_started) {
        component.m_started = false;
        guarded(component, "onStop", [&] {
            component.onStop();
        });
    }
    // Следующий запуск снова начнёт с onEnable и onStart, даже у компонента без фаз кадра.
    if (component.activeAndEnabled() && std::ranges::find(m_starting, &component) == m_starting.end()) {
        m_starting.push_back(&component);
    }
}

void Scene::stop()
{
    if (m_root) {
        m_root->stop();
    }
}

void Scene::advance(float realDelta)
{
    m_inFrame = true;
    startPending();
    m_time.beginFrame(realDelta);
    while (m_time.takeFixedStep()) {
        for (const auto& system : m_systems) {
            system->fixedUpdate(m_time.fixedStep());
        }
        runPhase(LifecyclePhase::FixedUpdate, m_time.fixedStep());
    }
    runPhase(LifecyclePhase::Update, m_time.delta());
    for (const auto& system : m_systems) {
        system->update(m_time.delta());
    }
    runPhase(LifecyclePhase::LateUpdate, m_time.delta());
    for (const auto& system : m_systems) {
        system->lateUpdate(m_time.delta());
    }
    m_inFrame = false;

    // Сначала дерево: уничтоженные уходят и из очереди включённых.
    for (const TreeChange& change : std::exchange(m_treeChanges, {})) {
        applyTreeChange(change);
    }
    for (Component* component : std::exchange(m_joining, {})) {
        activate(*component);
    }
}

void Scene::updateTransforms()
{
    for (const auto& system : m_systems) {
        system->updateTransforms();
    }
}

void Scene::startPending()
{
    // Очередь забираем целиком: onStart может добавить компоненты, они
    // запустятся в следующем кадре.
    m_startBatch = std::exchange(m_starting, {});
    for (std::size_t i = 0; i < m_startBatch.size(); ++i) {
        if (Component* component = m_startBatch[i]) {
            start(*component);
        }
    }
    m_startBatch.clear();
}

void Scene::start(Component& component)
{
    // Каждый вызов мог выключить компонент: исключением или из кода игры.
    if (!component.m_enableCalled && component.activeAndEnabled()) {
        component.m_enableCalled = true;
        guarded(component, "onEnable", [&] {
            component.onEnable();
        });
    }
    if (!component.m_started && component.activeAndEnabled()) {
        component.m_started = true;
        guarded(component, "onStart", [&] {
            component.onStart();
        });
    }
}

void Scene::runPhase(LifecyclePhase phase, float seconds)
{
    m_phases[static_cast<std::size_t>(phase)].forEach([&](Component& component) {
        // Включённый в этом кадре мог ещё не получить onEnable и onStart.
        start(component);
        if (!component.activeAndEnabled()) {
            return;
        }
        guarded(component, phaseCallName(phase), [&] {
            switch (phase) {
            case LifecyclePhase::FixedUpdate: component.onFixedUpdate(seconds); break;
            case LifecyclePhase::Update: component.onUpdate(seconds); break;
            case LifecyclePhase::LateUpdate: component.onLateUpdate(seconds); break;
            }
        });
    });
}

} // namespace yqlora
