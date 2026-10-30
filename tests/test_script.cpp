#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/input/InputSettings.hpp>
#include <yqlora/platform/InputFeeder.hpp>
#include <yqlora/reflection/PropertyJson.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>
#include <yqlora/scene/Transform3D.hpp>
#include <yqlora/scripting/Script.hpp>

#include <algorithm>
#include <string>

using namespace yqlora;

namespace {

// Скрипт как в игре: движение по действию ввода, поле с пределами и закрытое поле.
class Mover : public Script {
public:
    YQLORA_SCRIPT(Mover)

    float speed = 2.0f;
    float runtimeOnly = 0.0f;

    YQLORA_FIELDS(
        field("speed", &Mover::speed).range(0.0f, 10.0f).formerly("moveSpeed").group("movement"),
        field("bonus", &Mover::m_bonus).displayName("Bonus").tooltip("Extra speed"),
        field("runtimeOnly", &Mover::runtimeOnly).notSaved().notShown()
    )

    int updates = 0;
    std::string found;

protected:
    void onUpdate(float deltaSeconds) override
    {
        ++updates;
        transform().setPosition(
            transform().position() + Vec3(input().actionValue("MoveX") * speed * deltaSeconds, 0.0f, 0.0f)
        );
        if (const Node* target = findNode("Target")) {
            found = target->name();
        }
    }

private:
    float m_bonus = 1.0f;
};

struct Fixture {
    Fixture()
    {
        registerScript<Mover>(registry);
    }

    const Property& property(std::string_view name) const
    {
        const auto properties = registry.properties(*registry.find<Mover>());

        return **std::ranges::find(properties, name, &Property::name);
    }

    test::EngineRegistry registry;
    Input input;
    EngineServices services{input, registry};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "Script: registered as a script with its fields and their attributes")
{
    const TypeInfo& type = *registry.find<Mover>();
    CHECK(type.isScript());
    CHECK(type.allowsMultiple());
    CHECK(type.baseName() == "Script");
    CHECK_FALSE(registry.find<Script>()->creatable());

    const Property& speed = property("speed");
    CHECK(speed.attributes().min == 0.0f);
    CHECK(speed.attributes().max == 10.0f);
    CHECK(speed.attributes().group == "movement");
    CHECK(property("bonus").attributes().displayName == "Bonus");
    CHECK(property("bonus").attributes().tooltip == "Extra speed");
    CHECK_FALSE(property("runtimeOnly").attributes().saved);
    CHECK_FALSE(property("runtimeOnly").attributes().shown);

    // Закрытое поле читается и пишется так же, как открытое.
    Mover mover;
    CHECK(property("bonus").set(mover, PropertyValue{3.0f}));
    CHECK(property("bonus").get(mover) == PropertyValue{3.0f});
}

TEST_CASE_FIXTURE(Fixture, "Script: fields are saved by name, read under a former name, unsaved ones skipped")
{
    Mover mover;
    mover.speed = 4.0f;
    mover.runtimeOnly = 9.0f;
    const auto json = propertiesToJson(registry, *registry.find<Mover>(), mover);
    CHECK(json.contains("speed"));
    CHECK_FALSE(json.contains("runtimeOnly"));

    Mover loaded;
    propertiesFromJson(registry, *registry.find<Mover>(), loaded, nlohmann::json{{"moveSpeed", 7.0f}});
    CHECK(loaded.speed == 7.0f);
}

TEST_CASE_FIXTURE(Fixture, "Script: reaches the engine through its node and scene")
{
    InputSettings settings;
    InputFeeder feeder(input);
    feeder.keyDown(Key::D);
    feeder.updateActions(settings);

    Scene scene{services};
    auto& root = scene.createRoot<Node3D>("Root");
    root.addChild<Node3D>("Target");
    auto& mover = root.addComponent<Mover>();
    scene.advance(0.5f);

    CHECK(mover.updates == 1);
    CHECK(root.transform().position().x == doctest::Approx(1.0f));
    CHECK(mover.found == "Target");
}
