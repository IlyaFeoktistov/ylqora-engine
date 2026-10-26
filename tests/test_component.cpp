#include <doctest/doctest.h>

#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node2D.hpp>
#include <yqlora/scene/Node3D.hpp>

using namespace yqlora;

namespace {

struct Health final : Component {
    explicit Health(int initial = 100)
        : value(initial)
    {
    }
    std::string_view typeName() const override
    {
        return "Health";
    }

    int value;
};

} // namespace

TEST_CASE("Component: plain Node has no components")
{
    const Node node;
    CHECK(node.components().empty());
}

TEST_CASE("Component: Transform is the first component of every scene node")
{
    Node3D node3D;
    REQUIRE(node3D.components().size() == 1);
    CHECK(node3D.components()[0].get() == &node3D.transform());
    CHECK(node3D.components()[0]->typeName() == "Transform3D");
    CHECK(node3D.transform().owner() == &node3D);

    Node2D node2D;
    REQUIRE(node2D.components().size() == 1);
    CHECK(node2D.components()[0]->typeName() == "Transform2D");

    // Наследники получают тот же Transform от базового узла.
    Camera3D camera;
    CHECK(camera.getComponent<Transform3D>() == &camera.transform());
}

TEST_CASE("Component: add, find and remove")
{
    Node3D node;
    auto& health = node.addComponent<Health>(42);

    CHECK(health.owner() == &node);
    REQUIRE(node.components().size() == 2);
    CHECK(node.components()[1].get() == &health);
    CHECK(node.getComponent<Health>() == &health);
    CHECK(node.getComponent<Health>()->value == 42);

    auto owned = node.removeComponent(health);
    REQUIRE(owned);
    CHECK(owned->owner() == nullptr);
    CHECK(node.getComponent<Health>() == nullptr);

    Node other;
    CHECK(other.removeComponent(*owned) == nullptr);
}

TEST_CASE("Component: Transform cannot be removed")
{
    Node3D node;
    CHECK(node.transform().builtin());
    CHECK_FALSE(node.canRemoveComponent(node.transform()));
    CHECK(node.removeComponent(node.transform()) == nullptr);
    CHECK(node.components().size() == 1);
    CHECK(node.transform().owner() == &node);
}

TEST_CASE("Component: canRemoveComponent only for own non-builtin components")
{
    Node3D node;
    auto& health = node.addComponent<Health>();
    CHECK_FALSE(health.builtin());
    CHECK(node.canRemoveComponent(health));

    Node3D other;
    CHECK_FALSE(other.canRemoveComponent(health));
}
