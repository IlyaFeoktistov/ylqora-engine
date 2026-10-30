#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/SceneSystem.hpp"

#include <concepts>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace yqlora {

class Scene;
class TypeRegistry;

// Системы одной сцены: по одной на каждый тип систем в реестре, вызываются
// в порядке регистрации.
class YQLORA_API SceneSystems {
public:
    SceneSystems(Scene& scene, const TypeRegistry& registry);
    ~SceneSystems();

    SceneSystems(const SceneSystems&) = delete;
    SceneSystems& operator=(const SceneSystems&) = delete;

    // nullptr, если такой системы в сцене нет.
    template <std::derived_from<SceneSystem> T>
    T* get() const
    {
        const auto it = m_byType.find(std::type_index(typeid(T)));

        return it != m_byType.end() ? static_cast<T*>(it->second) : nullptr;
    }

    void fixedUpdate(float step);
    void update(float deltaSeconds);
    void lateUpdate(float deltaSeconds);
    void updateTransforms();

private:
    std::vector<std::unique_ptr<SceneSystem>> m_systems;
    std::unordered_map<std::type_index, SceneSystem*> m_byType;
};

} // namespace yqlora
