#include <doctest/doctest.h>

#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>

#include <string>
#include <vector>

using namespace yqlora;

namespace {

// Записывает, какие события жизненного цикла и в каком порядке пришли.
struct Recorder final : Component {
    explicit Recorder(std::vector<std::string>& log)
        : events(log)
    {
    }

    YQLORA_TYPE(Recorder)

    void onStart() override
    {
        events.push_back("start");
    }

    void onUpdate(float /*deltaSeconds*/) override
    {
        events.push_back("update");
    }

    void onStop() override
    {
        events.push_back("stop");
    }

    std::vector<std::string>& events;
};

} // namespace

TEST_CASE("Lifecycle: start before the first update, stop when the game stops")
{
    std::vector<std::string> events;
    Scene scene;
    scene.createRoot<Node3D>().addComponent<Recorder>(events);

    scene.update(0.016f);
    scene.update(0.016f);
    scene.stop();
    CHECK(events == std::vector<std::string>{"start", "update", "update", "stop"});

    // После остановки следующий запуск снова начинается с onStart.
    events.clear();
    scene.update(0.016f);
    CHECK(events == std::vector<std::string>{"start", "update"});
}

TEST_CASE("Lifecycle: a component added during the game starts on its first update")
{
    std::vector<std::string> events;
    Scene scene;
    auto& root = scene.createRoot<Node3D>();
    scene.update(0.016f);

    root.addComponent<Recorder>(events);
    scene.update(0.016f);
    CHECK(events == std::vector<std::string>{"start", "update"});
}

TEST_CASE("Lifecycle: removing a started component or node stops it")
{
    std::vector<std::string> events;
    Scene scene;
    auto& root = scene.createRoot<Node3D>();
    auto& child = root.addChild<Node3D>();
    auto& recorder = child.addComponent<Recorder>(events);
    scene.update(0.016f);

    events.clear();
    auto owned = child.removeComponent(recorder);
    CHECK(events == std::vector<std::string>{"stop"});

    child.addComponent(std::move(owned));
    scene.update(0.016f);
    events.clear();
    auto removedNode = root.removeChild(child);
    CHECK(events == std::vector<std::string>{"stop"});
    CHECK_FALSE(removedNode->started());
}

TEST_CASE("Lifecycle: nothing happens without updates")
{
    std::vector<std::string> events;
    Scene scene;
    scene.createRoot<Node3D>().addComponent<Recorder>(events);
    scene.stop();
    CHECK(events.empty());
}
