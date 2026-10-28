#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace yqlora {

class Component;

// Фазы кадра, в которых сцена вызывает компоненты, см. Scene::advance.
enum class LifecyclePhase : std::uint8_t {
    FixedUpdate,
    Update,
    LateUpdate,
};

inline constexpr std::size_t kLifecyclePhaseCount = 3;

// Набор фаз: бит i - фаза i.
using LifecyclePhases = std::uint8_t;

constexpr LifecyclePhases phaseBit(LifecyclePhase phase)
{
    return static_cast<LifecyclePhases>(1u << static_cast<unsigned>(phase));
}

// Какие фазы кадра переопределил тип компонента. Метод фазы переопределён,
// если объявлен не в самом Component: тогда у указателя на метод другой класс.
// YQLORA_TYPE делает эту структуру другом класса, иначе к защищённым onUpdate
// и остальным снаружи не дотянуться.
struct PhaseDetector {
    template <typename T>
    static constexpr LifecyclePhases phasesOf()
    {
        LifecyclePhases result = 0;
        if (overrides<decltype(&T::onFixedUpdate)>()) {
            result |= phaseBit(LifecyclePhase::FixedUpdate);
        }
        if (overrides<decltype(&T::onUpdate)>()) {
            result |= phaseBit(LifecyclePhase::Update);
        }
        if (overrides<decltype(&T::onLateUpdate)>()) {
            result |= phaseBit(LifecyclePhase::LateUpdate);
        }

        return result;
    }

private:
    template <typename Method>
    static constexpr bool overrides()
    {
        return !std::is_same_v<Method, void (Component::*)(float)>;
    }
};

} // namespace yqlora
