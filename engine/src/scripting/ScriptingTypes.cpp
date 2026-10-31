#include "yqlora/scripting/ScriptingTypes.hpp"

#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scripting/Script.hpp"

namespace yqlora {

void registerScriptingTypes(TypeRegistry& registry)
{
    // База в реестре нужна, чтобы скрипты игры встали в цепочку типов; сама
    // она не создаётся и в "Добавить компонент" не попадает.
    registry.registerComponent<Script>();
}

} // namespace yqlora
