// Модуль игры для тестов GameModule: собирается отдельной библиотекой, как
// game.dll проекта.

#include <yqlora/scene/Node.hpp>
#include <yqlora/scripting/ModuleScripts.hpp>
#include <yqlora/scripting/Script.hpp>

#include <string>

namespace {

// Отмечает вызовы жизненного цикла в имени своего узла: узел принадлежит
// движку и остаётся, когда скрипт уже уничтожен, а библиотека выгружена.
class TestMover : public yqlora::Script {
public:
    YQLORA_SCRIPT(TestMover)

    float speed = 1.0f;
    int updates = 0;

    YQLORA_FIELDS(field("speed", &TestMover::speed), field("updates", &TestMover::updates).notSaved())

protected:
    void onStart() override
    {
        node().setName(node().name() + kStarted);
    }

    void onUpdate(float /*deltaSeconds*/) override
    {
        ++updates;
    }

    void onStop() override
    {
        node().setName(node().name() + kStopped);
    }

private:
    static constexpr const char* kStarted = "+start";
    static constexpr const char* kStopped = "+stop";
};

YQLORA_REGISTER_SCRIPT(TestMover);

} // namespace
