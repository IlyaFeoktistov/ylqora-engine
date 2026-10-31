// Входная функция модуля игры, которую yqlora_add_game_module добавляет к
// каждому модулю: регистрирует скрипты, отмеченные YQLORA_REGISTER_SCRIPT.

#include <yqlora/scripting/ModuleScripts.hpp>

YQLORA_MODULE_ENTRY(registrar)
{
    yqlora::registerModuleScripts(registrar);
}
