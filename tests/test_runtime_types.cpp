#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <map>
#include <string>
#include <string_view>

using namespace yqlora;

namespace {

// Как будущий LuauScript: один C++-класс на все типы скриптов, тип и
// значения свойств хранит сам экземпляр.
class ScriptStub final : public Component {
public:
    explicit ScriptStub(std::string type)
        : m_type(std::move(type))
    {
    }

    std::string_view typeName() const override
    {
        return m_type;
    }

    std::map<std::string, PropertyValue, std::less<>> values;
    int updates = 0;

protected:
    void onUpdate(float /*deltaSeconds*/) override
    {
        ++updates;
    }

private:
    std::string m_type;
};

constexpr std::string_view kSpinnerType = "Spinner";
constexpr std::string_view kSpeedProperty = "speed";
constexpr float kDefaultSpeed = 1.0f;
constexpr float kChangedSpeed = 4.5f;
constexpr float kFrame = 0.016f;

ScriptStub& stub(Component& component)
{
    return static_cast<ScriptStub&>(component);
}

const ScriptStub& stub(const Component& component)
{
    return static_cast<const ScriptStub&>(component);
}

struct Fixture {
    Fixture()
    {
        registry.registerType(std::string(kSpinnerType), TypeKind::Component, std::string(typeNameOf<Component>()))
            .createComponent([] {
                auto script = std::make_unique<ScriptStub>(std::string(kSpinnerType));
                script->values.emplace(kSpeedProperty, kDefaultSpeed);

                return script;
            })
            .phases(phaseBit(LifecyclePhase::Update))
            .property(
                Property::forComponent(
                    std::string(kSpeedProperty),
                    PropertyType::Float,
                    [](const Component& component) {
                        return stub(component).values.find(kSpeedProperty)->second;
                    },
                    [](Component& component, const PropertyValue& value) {
                        stub(component).values.find(kSpeedProperty)->second = value;
                    }
                )
            );
    }

    const TypeInfo& spinner() const
    {
        return *registry.find(kSpinnerType);
    }

    test::EngineRegistry registry;
    Input input;
    EngineServices services{input, registry};
    SceneSerializer serializer{registry};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "Runtime type: registered without a C++ class of its own")
{
    const TypeInfo& type = spinner();
    CHECK(type.kind() == TypeKind::Component);
    CHECK(type.creatable());
    CHECK(type.phases() == phaseBit(LifecyclePhase::Update));
    // Свойства корня Component на месте, как у типов из C++.
    std::vector<std::string> names;
    for (const Property* property : registry.properties(type)) {
        names.push_back(property->name());
    }
    CHECK(names == std::vector<std::string>{"enabled", std::string(kSpeedProperty)});
    // Своего C++-класса нет: по классу его не найти.
    CHECK(registry.find<ScriptStub>() == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "Runtime type: takes part in the frame and is found in the scene by its name")
{
    Scene scene{services};
    auto& root = scene.createRoot<Node3D>();
    Component& component = root.addComponent(spinner().createComponent());

    scene.advance(kFrame);
    CHECK(stub(component).updates == 1);
    REQUIRE(scene.componentsOfType(kSpinnerType).size() == 1);
    CHECK(scene.componentsOfType(kSpinnerType)[0] == &component);
}

TEST_CASE_FIXTURE(Fixture, "Runtime type: its properties survive save and load")
{
    Scene original{services};
    auto& root = original.createRoot<Node3D>();
    Component& component = root.addComponent(spinner().createComponent());
    const Property& speed = *registry.properties(spinner()).back();
    REQUIRE(speed.set(component, kChangedSpeed));

    Scene loaded{services};
    REQUIRE(serializer.fromJson(loaded, serializer.toJson(original)));
    const auto loadedComponents = loaded.root()->components();
    const auto it = std::ranges::find_if(loadedComponents, [](const auto& c) {
        return c->typeName() == kSpinnerType;
    });
    REQUIRE(it != loadedComponents.end());
    CHECK(std::get<float>(speed.get(**it)) == doctest::Approx(kChangedSpeed));
}
