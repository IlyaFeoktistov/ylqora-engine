#include "EngineRegistry.hpp"

#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/editor/SceneCommands.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>
#include <yqlora/scripting/GameModule.hpp>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

// Корень, под ним сдвинутая и повёрнутая рука и рядом три меча; меч в
// середине, чтобы было видно, что отмена возвращает его на то же место.
struct Fixture {
    Fixture()
    {
        root = static_cast<Node3D*>(session.editing().createNode(*registry.find("Node3D"), nullptr));
        hand = static_cast<Node3D*>(session.editing().createNode(*registry.find("Node3D"), root));
        hand->transform().setPosition({5.0f, 0.0f, 0.0f});
        hand->transform().setRotationDegrees({0.0f, 90.0f, 0.0f});
        first = static_cast<Node3D*>(session.editing().createNode(*registry.find("Node3D"), root));
        sword = static_cast<Node3D*>(session.editing().createNode(*registry.find("Node3D"), root));
        session.editing().createNode(*registry.find("Node3D"), root);
        sword->transform().setPosition({1.0f, 2.0f, 3.0f});
        session.scene().updateTransforms();
        session.selection().select(sword->id());
    }

    test::EngineRegistry registry;
    SceneSerializer serializer{registry};
    Input input;
    EngineServices services{input, registry};
    GameModule gameModule{registry};
    EditorSession session{services, serializer, gameModule};
    Node3D* root = nullptr;
    Node3D* hand = nullptr;
    Node3D* first = nullptr;
    Node3D* sword = nullptr;
};

std::size_t indexOf(const Node& node)
{
    const auto siblings = node.parent()->children();
    for (std::size_t i = 0; i < siblings.size(); ++i) {
        if (siblings[i].get() == &node) {
            return i;
        }
    }

    return siblings.size();
}

void checkNear(const Vec3& a, const Vec3& b)
{
    CHECK(a.x == doctest::Approx(b.x));
    CHECK(a.y == doctest::Approx(b.y));
    CHECK(a.z == doctest::Approx(b.z));
}

} // namespace

TEST_CASE_FIXTURE(Fixture, "SetParentCommand: keeping the world position, undo returns the parent, place and values")
{
    const TransformValues before = sword->transform().values();
    REQUIRE(indexOf(*sword) == 2);
    REQUIRE(session.editing().setParent(*sword, *hand, KeepWorld::Yes));
    session.scene().updateTransforms();
    CHECK(sword->parent() == hand);
    checkNear(sword->worldPosition(), Vec3(1.0f, 2.0f, 3.0f));
    CHECK(session.selection().selected() == sword->id());

    REQUIRE(session.undo());
    session.scene().updateTransforms();
    CHECK(sword->parent() == root);
    CHECK(indexOf(*sword) == 2);
    CHECK(sword->transform().values() == before);
    CHECK(session.selection().selected() == sword->id());

    REQUIRE(session.redo());
    session.scene().updateTransforms();
    CHECK(sword->parent() == hand);
    checkNear(sword->worldPosition(), Vec3(1.0f, 2.0f, 3.0f));
}

TEST_CASE_FIXTURE(Fixture, "SetParentCommand: keeping the local values moves the node with its new parent")
{
    REQUIRE(session.editing().setParent(*sword, *hand, KeepWorld::No));
    session.scene().updateTransforms();
    CHECK(sword->transform().position() == Vec3(1.0f, 2.0f, 3.0f));
    checkNear(sword->worldPosition(), hand->toWorld(Vec3(1.0f, 2.0f, 3.0f)));
}

TEST_CASE_FIXTURE(Fixture, "canMoveUnder: not the root, not into its own branch, not to the same parent")
{
    CHECK(canMoveUnder(*sword, *hand));
    CHECK_FALSE(canMoveUnder(*root, *hand));
    CHECK_FALSE(canMoveUnder(*sword, *root));
    REQUIRE(session.editing().setParent(*sword, *hand, KeepWorld::No));
    CHECK_FALSE(canMoveUnder(*hand, *sword));
    CHECK_FALSE(canMoveUnder(*sword, *sword));
    CHECK_FALSE(session.editing().setParent(*hand, *sword, KeepWorld::No));
    CHECK(hand->parent() == root);
}
