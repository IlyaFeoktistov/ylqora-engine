#pragma once

namespace yqlora {

class TypeRegistry;

// Camera3D, MeshRenderer и RenderSystem. Типы сцены должны быть уже зарегистрированы.
void registerRenderTypes(TypeRegistry& registry);

} // namespace yqlora
