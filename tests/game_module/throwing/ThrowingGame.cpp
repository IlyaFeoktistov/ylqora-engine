// Модуль игры, входная функция которого падает посреди регистрации: после
// неё в реестре не должно остаться ни одного его типа.

#include <yqlora/scripting/ModuleRegistrar.hpp>
#include <yqlora/scripting/Script.hpp>

#include <stdexcept>

namespace {

class Registered : public yqlora::Script {
public:
    YQLORA_SCRIPT(Registered)
};

} // namespace

YQLORA_MODULE_ENTRY(registrar)
{
    registrar.registerScript<Registered>();
    throw std::runtime_error("broken registration");
}
