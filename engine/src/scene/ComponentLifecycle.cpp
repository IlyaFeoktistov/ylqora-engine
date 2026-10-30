#include "yqlora/scene/ComponentLifecycle.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/ExecutionOrder.hpp"
#include "yqlora/scene/Node.hpp"

#include <algorithm>
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

ComponentLifecycle::ComponentLifecycle(const EngineServices& services)
    : m_services(services)
{
}

void ComponentLifecycle::activate(Component& component)
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

void ComponentLifecycle::deactivate(Component& component)
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

void ComponentLifecycle::stop(Component& component)
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

void ComponentLifecycle::beginFrame()
{
    m_inFrame = true;
    startPending();
}

void ComponentLifecycle::endFrame()
{
    m_inFrame = false;
}

void ComponentLifecycle::activateJoined()
{
    for (Component* component : std::exchange(m_joining, {})) {
        activate(*component);
    }
}

void ComponentLifecycle::startPending()
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

void ComponentLifecycle::start(Component& component)
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

void ComponentLifecycle::runPhase(LifecyclePhase phase, float seconds)
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
