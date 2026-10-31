// Модуль игры, регистрация которого падает посреди дела: после неё в реестре
// не должно остаться ни одного его типа.

#include <yqlora/scripting/ModuleScripts.hpp>

#include <memory>
#include <stdexcept>

namespace {

// По имени регистрируется первым, до падения.
class Registered : public yqlora::Script {
public:
    YQLORA_SCRIPT(Registered)
};

YQLORA_REGISTER_SCRIPT(Registered);

// Запись модуля, которая бросает исключение вместо регистрации.
class Throwing final : public yqlora::detail::ModuleScript {
public:
    std::string_view name() const override
    {
        return "Throwing";
    }

    void registerIn(yqlora::ModuleRegistrar& /*registrar*/) const override
    {
        throw std::runtime_error("broken registration");
    }
};

[[maybe_unused]] const bool kThrowingAdded = [] {
    yqlora::detail::moduleScripts().push_back(std::make_unique<Throwing>());

    return true;
}();

} // namespace
