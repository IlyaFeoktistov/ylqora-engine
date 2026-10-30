#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/LifecyclePhase.hpp"
#include "yqlora/scene/PhaseList.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace yqlora {

class Component;
class EngineServices;

// Расписание жизненного цикла компонентов сцены: списки фаз кадра, очереди
// onEnable / onStart, onDisable / onStop. Исключение из вызова не роняет
// игру: компонент выключается, ошибка уходит в лог.
class YQLORA_API ComponentLifecycle {
public:
    explicit ComponentLifecycle(const EngineServices& services);

    // Идёт кадр: между beginFrame и endFrame.
    bool inFrame() const
    {
        return m_inFrame;
    }

    // Компонент начал участвовать в игре (включён, узел активен). Во время
    // кадра - с конца кадра, см. activateJoined.
    void activate(Component& component);
    // Перестал участвовать: уходит из списков, получает onDisable.
    void deactivate(Component& component);
    // Игра остановлена: onDisable и onStop. Следующий запуск снова начнёт
    // с onEnable и onStart.
    void stop(Component& component);

    // Начало кадра: порядок выполнения заново, если настройки проекта
    // изменились, затем запуск ждущих onEnable и onStart.
    void beginFrame();
    void runPhase(LifecyclePhase phase, float seconds);
    void endFrame();
    // Включённые во время кадра встают в списки фаз.
    void activateJoined();

private:
    void startPending();
    // Порядок выполнения из настроек проекта берётся, когда компонент входит
    // в списки; после правки настроек списки пересортировываются.
    void refreshExecutionOrder();
    int orderOf(const Component& component) const;
    void start(Component& component);

    const EngineServices& m_services;
    std::array<PhaseList, kLifecyclePhaseCount> m_phases;
    // Ждут onEnable и onStart в начале следующего кадра.
    std::vector<Component*> m_starting;
    // Запускаются сейчас; снятые во время запуска заменяются на nullptr.
    std::vector<Component*> m_startBatch;
    // Включены во время кадра, встанут в списки после него.
    std::vector<Component*> m_joining;
    bool m_inFrame = false;
    // С каких настроек взят порядок выполнения в списках.
    const void* m_orderProject = nullptr;
    std::uint64_t m_orderRevision = 0;
};

} // namespace yqlora
