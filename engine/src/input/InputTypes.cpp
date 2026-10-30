#include "yqlora/input/InputTypes.hpp"

#include "yqlora/input/InputSettings.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"

namespace yqlora {

void registerInputTypes(TypeRegistry& registry)
{
    registry.registerSettings<InputSettings>("input").property("actions", &InputSettings::actions);
}

} // namespace yqlora
