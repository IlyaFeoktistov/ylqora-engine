// Входная функция модуля игры: единственное место, где регистрируются скрипты.

#include "Pulse.hpp"

#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scripting/ModuleRegistrar.hpp>

YQLORA_MODULE_ENTRY(registrar)
{
    registrar.registerScript<Pulse>().requiresNode<yqlora::Node3D>();
}
