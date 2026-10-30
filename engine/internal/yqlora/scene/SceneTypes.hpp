#pragma once

namespace yqlora {

class TypeRegistry;

// Корни Node, Component, SceneSystem, узел Node3D с Transform3D и система трансформов.
void registerSceneTypes(TypeRegistry& registry);

} // namespace yqlora
