#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>

#include <string>
#include <vector>

using namespace yqlora;

namespace {

using Events = std::vector<std::string>;

// Записывает, какие события жизненного цикла и в каком порядке пришли.
struct Recorder : Component {
    Recorder(Events& log, std::string name = "")
        : events(log)
        , prefix(std::move(name))
    {
    }

    YQLORA_TYPE(Recorder, Component)

    void onStart() override
    {
        events.push_back(prefix + "start");
    }

    void onFixedUpdate(float /*stepSeconds*/) override
    {
        events.push_back(prefix + "fixed");
    }

    void onUpdate(float /*deltaSeconds*/) override
    {
        events.push_back(prefix + "update");
    }

    void onLateUpdate(float /*deltaSeconds*/) override
    {
        events.push_back(prefix + "late");
    }

    void onStop() override
    {
        events.push_back(prefix + "stop");
    }

    Events& events;
    std::string prefix;
};

// Только onUpdate: в списках других фаз его быть не должно.
struct UpdateOnly final : Component {
    explicit UpdateOnly(Events& log)
        : events(log)
    {
    }

    YQLORA_TYPE(UpdateOnly, Component)

    void onUpdate(float /*deltaSeconds*/) override
    {
        events.push_back("update");
    }

    Events& events;
};

// Порядок выполнения раньше остальных.
struct Early final : Recorder {
    using Recorder::Recorder;
    YQLORA_TYPE(Early, Recorder)
};

// Шаг кадра больше фиксированного шага ровно вдвое: два фиксированных шага.
constexpr float kTwoFixedSteps = 2.0f * kDefaultFixedStep;
// Десятая доля фиксированного шага: тест делает меньше десяти таких кадров
// подряд, и целый шаг в накопителе не набирается.
constexpr float kNoFixedStep = 0.1f * kDefaultFixedStep;

struct Fixture {
    Fixture()
    {
        registerEngineTypes(registry);
        registry.registerComponent<Recorder>().allowMultiple();
        registry.registerComponent<UpdateOnly>();
        registry.registerComponent<Early>().executionOrder(-1);
    }

    // Журнал раньше сцены: сцена при уничтожении останавливает дерево и пишет в него.
    Events events;
    TypeRegistry registry;
    Input input;
    EngineServices services{input, registry};
    Scene scene{services};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "Lifecycle: start once, then fixed steps, update and late update")
{
    scene.createRoot<Node3D>().addComponent<Recorder>(events);

    scene.advance(kTwoFixedSteps);
    CHECK(events == Events{"start", "fixed", "fixed", "update", "late"});

    events.clear();
    scene.advance(kNoFixedStep);
    CHECK(events == Events{"update", "late"});

    events.clear();
    scene.stop();
    CHECK(events == Events{"stop"});

    // После остановки следующий запуск снова начинается с onStart.
    events.clear();
    scene.advance(kNoFixedStep);
    CHECK(events == Events{"start", "update", "late"});
}

TEST_CASE_FIXTURE(Fixture, "Lifecycle: only overridden phases are called")
{
    scene.createRoot<Node3D>().addComponent<UpdateOnly>(events);
    CHECK(registry.find<UpdateOnly>()->phases() == phaseBit(LifecyclePhase::Update));

    scene.advance(kTwoFixedSteps);
    CHECK(events == Events{"update"});
}

TEST_CASE_FIXTURE(Fixture, "Lifecycle: execution order, then the order of entering")
{
    auto& root = scene.createRoot<Node3D>();
    root.addComponent<Recorder>(events, "a:");
    root.addComponent<Early>(events, "early:");
    root.addComponent<Recorder>(events, "b:");
    scene.advance(kNoFixedStep);

    // onStart - в порядке входа, фазы - по порядку выполнения типа.
    CHECK(
        events
        == Events{
            "a:start",
            "early:start",
            "b:start",
            "early:update",
            "a:update",
            "b:update",
            "early:late",
            "a:late",
            "b:late"
        }
    );
}

TEST_CASE_FIXTURE(Fixture, "Lifecycle: a component added during the game starts on its first call")
{
    auto& root = scene.createRoot<Node3D>();
    scene.advance(kNoFixedStep);

    root.addComponent<Recorder>(events);
    scene.advance(kNoFixedStep);
    CHECK(events == Events{"start", "update", "late"});
}

TEST_CASE_FIXTURE(Fixture, "Lifecycle: removing a started component or node stops it")
{
    auto& root = scene.createRoot<Node3D>();
    auto& child = root.addChild<Node3D>();
    auto& recorder = child.addComponent<Recorder>(events);
    scene.advance(kNoFixedStep);

    events.clear();
    auto owned = child.removeComponent(recorder);
    CHECK(events == Events{"stop"});
    scene.advance(kNoFixedStep);
    CHECK(events == Events{"stop"});

    child.addComponent(std::move(owned));
    scene.advance(kNoFixedStep);
    events.clear();
    auto removedNode = root.removeChild(child);
    CHECK(events == Events{"stop"});
}

TEST_CASE_FIXTURE(Fixture, "Lifecycle: taking the tree out of the scene stops it")
{
    scene.createRoot<Node3D>().addComponent<Recorder>(events);
    scene.advance(kNoFixedStep);

    events.clear();
    auto root = scene.takeRoot();
    CHECK(events == Events{"stop"});
}

TEST_CASE_FIXTURE(Fixture, "Lifecycle: nothing happens without advance")
{
    scene.createRoot<Node3D>().addComponent<Recorder>(events);
    scene.stop();
    CHECK(events.empty());
}

TEST_CASE("Lifecycle: a scene without a registry calls nobody")
{
    Events events;
    Scene scene{EngineServices::none()};
    scene.createRoot<Node3D>().addComponent<UpdateOnly>(events);
    scene.advance(kNoFixedStep);
    CHECK(events.empty());
}

TEST_CASE("GameTime: scale, fixed steps and their limit")
{
    GameTime time;
    time.beginFrame(0.1f);
    CHECK(time.delta() == doctest::Approx(0.1f));
    CHECK(time.frame() == 1);

    time.setScale(0.5f);
    time.beginFrame(0.1f);
    CHECK(time.delta() == doctest::Approx(0.05f));
    CHECK(time.unscaledDelta() == doctest::Approx(0.1f));
    CHECK(time.elapsed() == doctest::Approx(0.15));

    // Стоп: время стоит, шагов нет.
    GameTime stopped;
    stopped.setScale(0.0f);
    stopped.beginFrame(1.0f);
    CHECK(stopped.delta() == 0.0f);
    CHECK_FALSE(stopped.takeFixedStep());

    // После очень длинного кадра шагов не больше предела.
    GameTime slow;
    slow.beginFrame(10.0f);
    int steps = 0;
    while (slow.takeFixedStep()) {
        ++steps;
    }
    CHECK(steps == kMaxFixedStepsPerFrame);
}
