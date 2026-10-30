#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using namespace yqlora;

namespace {

constexpr std::string_view kChildName = "Child";
const Vec3 kParentPosition{1.0f, 2.0f, 3.0f};
const Vec3 kLocalPoint{0.0f, 0.0f, -1.0f};
const Vec3 kTarget{0.0f, 0.0f, 5.0f};

struct Fixture {
    const Method& method(const TypeInfo& type, std::string_view name) const
    {
        const auto methods = registry.methods(type);
        const auto it = std::ranges::find_if(methods, [&](const Method* m) {
            return m->name() == name;
        });
        REQUIRE(it != methods.end());

        return **it;
    }

    template <typename T>
    const Method& method(std::string_view name) const
    {
        return method(*registry.find<T>(), name);
    }

    test::EngineRegistry registry;
    Input input;
    EngineServices services{input, registry};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "Methods: a type has the methods of its bases, from the root")
{
    std::vector<std::string> names;
    for (const Method* m : registry.methods(*registry.find<Camera3D>())) {
        names.push_back(m->name());
    }
    REQUIRE_FALSE(names.empty());
    CHECK(names.front() == "findChild");
    CHECK(std::ranges::find(names, "worldPosition") != names.end());
}

TEST_CASE_FIXTURE(Fixture, "Methods: types of parameters and result come from the C++ signature")
{
    const Method& toWorld = method<Node3D>("toWorld");
    REQUIRE(toWorld.parameters().size() == 1);
    CHECK(toWorld.parameters()[0].name == "localPoint");
    CHECK(toWorld.parameters()[0].type == PropertyType::Vector3);
    CHECK(toWorld.result() == PropertyType::Vector3);

    const Method& setWorldPosition = method<Node3D>("setWorldPosition");
    CHECK_FALSE(setWorldPosition.result().has_value());

    // Указатель на узел возвращается ссылкой на узел.
    const Method& findChild = method<Node>("findChild");
    CHECK(findChild.parameters()[0].type == PropertyType::String);
    CHECK(findChild.result() == PropertyType::Node);
}

TEST_CASE_FIXTURE(Fixture, "Methods: call with values gives the same as the C++ call")
{
    Scene scene{services};
    auto& parent = scene.createRoot<Node3D>();
    parent.transform().setPosition(kParentPosition);
    auto& child = parent.addChild<Node3D>(std::string(kChildName));

    const std::vector<PropertyValue> point{kLocalPoint};
    const MethodCallResult world = method<Node3D>("toWorld").call(child, point);
    REQUIRE(world.called);
    CHECK(math::nearlyEqual(std::get<Vec3>(*world.value), child.toWorld(kLocalPoint)));

    const std::vector<PropertyValue> position{kTarget};
    const MethodCallResult moved = method<Node3D>("setWorldPosition").call(child, position);
    CHECK(moved.called);
    CHECK_FALSE(moved.value.has_value());
    CHECK(math::nearlyEqual(child.worldPosition(), kTarget));

    const std::vector<PropertyValue> name{std::string(kChildName)};
    const MethodCallResult found = method<Node>("findChild").call(parent, name);
    CHECK(std::get<NodeRef>(*found.value) == NodeRef{child.id()});
    const std::vector<PropertyValue> missing{std::string("Missing")};
    CHECK(std::get<NodeRef>(*method<Node>("findChild").call(parent, missing).value) == NodeRef{});
}

TEST_CASE_FIXTURE(Fixture, "Methods: missing arguments take their defaults")
{
    Scene scene{services};
    auto& node = scene.createRoot<Node3D>();
    const Method& lookAt = method<Transform3D>("lookAt");
    CHECK(lookAt.parameters()[1].defaultValue == PropertyValue(math::kWorldUp));

    const std::vector<PropertyValue> target{kTarget};
    REQUIRE(lookAt.call(node.transform(), target).called);
    CHECK(math::nearlyEqual(node.worldForward(), glm::normalize(kTarget)));
}

TEST_CASE_FIXTURE(Fixture, "Methods: wrong arguments are not passed to the method")
{
    Scene scene{services};
    auto& node = scene.createRoot<Node3D>();
    const Method& setWorldPosition = method<Node3D>("setWorldPosition");

    const std::vector<PropertyValue> none;
    const std::vector<PropertyValue> wrongType{1.0f};
    const std::vector<PropertyValue> tooMany{kTarget, kTarget};
    CHECK_FALSE(setWorldPosition.call(node, none).called);
    CHECK_FALSE(setWorldPosition.call(node, wrongType).called);
    CHECK_FALSE(setWorldPosition.call(node, tooMany).called);
    CHECK(math::nearlyEqual(node.worldPosition(), Vec3{0.0f}));
}
