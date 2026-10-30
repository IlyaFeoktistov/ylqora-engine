#include <doctest/doctest.h>

#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/editor/EditHistory.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/render/Camera3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <string_view>
#include <vector>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

struct AddCommand final : Command {
    AddCommand(int& target, int value)
        : counter(target)
        , amount(value)
    {
    }

    bool apply(EditorSession& /*session*/) override
    {
        counter += amount;

        return true;
    }

    void revert(EditorSession& /*session*/) override
    {
        counter -= amount;
    }

    bool merge(const Command& next) override
    {
        const auto* other = dynamic_cast<const AddCommand*>(&next);
        if (!other) {
            return false;
        }
        amount += other->amount;

        return true;
    }

    int& counter;
    int amount;
};

struct TestComponent final : Component {
    YQLORA_TYPE(TestComponent, Component)
};

struct Fixture {
    Fixture()
    {
        registerEngineTypes(registry);
        // Несколько на узле: тесты проверяют порядок одинаковых компонентов.
        registry.registerComponent<TestComponent>().allowMultiple();
    }

    const TypeInfo& type(std::string_view name) const
    {
        return *registry.find(name);
    }

    const Property& property(std::string_view typeName, std::string_view name) const
    {
        for (const Property* property : registry.properties(type(typeName))) {
            if (property->name() == name) {
                return *property;
            }
        }
        FAIL("no such property");

        return *registry.properties(type(typeName)).front();
    }

    TypeRegistry registry;
    SceneSerializer serializer{registry};
    Scene scene{EngineServices::none()};
    EditorSession session{scene, serializer};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "EditHistory: undo, redo and a new command drops the undone ones")
{
    EditHistory history;
    int counter = 0;
    history.execute(session, std::make_unique<AddCommand>(counter, 1));
    history.finishEdit();
    history.execute(session, std::make_unique<AddCommand>(counter, 10));
    CHECK(counter == 11);

    REQUIRE(history.undo(session));
    CHECK(counter == 1);
    REQUIRE(history.redo(session));
    CHECK(counter == 11);
    CHECK_FALSE(history.redo(session));

    history.undo(session);
    history.execute(session, std::make_unique<AddCommand>(counter, 100));
    CHECK(counter == 101);
    CHECK_FALSE(history.canRedo());
}

TEST_CASE_FIXTURE(Fixture, "EditHistory: commands of one edit merge until finishEdit")
{
    EditHistory history;
    int counter = 0;
    history.execute(session, std::make_unique<AddCommand>(counter, 1));
    history.execute(session, std::make_unique<AddCommand>(counter, 2));
    history.finishEdit();
    history.execute(session, std::make_unique<AddCommand>(counter, 4));
    CHECK(counter == 7);

    history.undo(session);
    CHECK(counter == 3);
    history.undo(session);
    CHECK(counter == 0);
    CHECK_FALSE(history.canUndo());
}

TEST_CASE_FIXTURE(Fixture, "EditHistory: the saved state is found by its place in history")
{
    EditHistory history;
    int counter = 0;
    CHECK(history.atSaved());

    history.execute(session, std::make_unique<AddCommand>(counter, 1));
    CHECK_FALSE(history.atSaved());
    history.markSaved();
    CHECK(history.atSaved());

    history.undo(session);
    CHECK_FALSE(history.atSaved());
    history.redo(session);
    CHECK(history.atSaved());
}

TEST_CASE_FIXTURE(Fixture, "EditHistory: the saved state can become unreachable")
{
    EditHistory history;
    int counter = 0;

    // Сохранили посреди правки, а потом она слилась дальше.
    history.execute(session, std::make_unique<AddCommand>(counter, 1));
    history.markSaved();
    history.execute(session, std::make_unique<AddCommand>(counter, 1));
    history.undo(session);
    history.redo(session);
    CHECK_FALSE(history.atSaved());

    // Отменили сохранённую команду и сделали другую вместо неё.
    history.finishEdit();
    history.execute(session, std::make_unique<AddCommand>(counter, 1));
    history.markSaved();
    history.undo(session);
    history.execute(session, std::make_unique<AddCommand>(counter, 5));
    history.undo(session);
    history.redo(session);
    CHECK_FALSE(history.atSaved());
}

TEST_CASE_FIXTURE(Fixture, "Undo: a created node goes away and comes back with the same id and place")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    session.createNode(type("Node3D"), root);
    Node* cube = session.createNode(type("Camera3D"), root);
    const NodeId cubeId = cube->id();
    REQUIRE(session.document().modified());

    REQUIRE(session.undo());
    CHECK(session.findNode(cubeId) == nullptr);
    CHECK(root->children().size() == 1);

    REQUIRE(session.redo());
    Node* again = session.findNode(cubeId);
    REQUIRE(again);
    CHECK(root->children()[1].get() == again);
    CHECK(session.selection().selected() == cubeId);

    // Отменили всё: сцена снова пуста и не изменена.
    while (session.undo()) {
    }
    CHECK(scene.empty());
    CHECK_FALSE(session.document().modified());
}

TEST_CASE_FIXTURE(Fixture, "Undo: a deleted branch returns to its place with the selection")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    Node* first = session.createNode(type("Node3D"), root);
    session.createNode(type("Node3D"), root);
    auto* camera = static_cast<Camera3D*>(session.createNode(type("Camera3D"), first));
    session.selection().select(camera->id());
    const NodeId firstId = first->id();

    REQUIRE(session.deleteNode(firstId));
    CHECK(session.selection().selected() == 0);

    REQUIRE(session.undo());
    REQUIRE(session.findNode(firstId));
    CHECK(root->children()[0]->id() == firstId);
    CHECK(session.selection().selected() == camera->id());
}

TEST_CASE_FIXTURE(Fixture, "Undo: deleting the root and undoing it brings the whole scene back")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    session.createNode(type("Node3D"), root);
    const NodeId rootId = root->id();

    REQUIRE(session.deleteNode(rootId));
    CHECK(scene.empty());
    REQUIRE(session.undo());
    REQUIRE(scene.root());
    CHECK(scene.root()->id() == rootId);
    CHECK(scene.root()->children().size() == 1);
}

TEST_CASE_FIXTURE(Fixture, "Undo: components come back to their place in the list")
{
    Node* node = session.createNode(type("Node3D"), nullptr);
    Component* a = session.addComponent(*node, type("TestComponent"));
    session.addComponent(*node, type("TestComponent"));
    REQUIRE(node->components().size() == 3);

    REQUIRE(session.removeComponent(*node, *a));
    CHECK(node->components().size() == 2);
    REQUIRE(session.undo());
    REQUIRE(node->components().size() == 3);
    CHECK(node->components()[1].get() == a);

    session.undo();
    session.undo();
    CHECK(node->components().size() == 1);
    session.redo();
    CHECK(node->components()[1].get() == a);
}

TEST_CASE_FIXTURE(Fixture, "Undo: dragging a value is one step, the next drag is another")
{
    auto* node = static_cast<Node3D*>(session.createNode(type("Node3D"), nullptr));
    Component& transform = node->transform();
    const Property& position = property("Transform3D", "position");
    session.finishEdit();

    session.setProperty(transform, position, Vec3(1.0f, 0.0f, 0.0f));
    session.setProperty(transform, position, Vec3(2.0f, 0.0f, 0.0f));
    session.finishEdit();
    session.setProperty(transform, position, Vec3(3.0f, 0.0f, 0.0f));
    CHECK(node->transform().position() == Vec3(3.0f, 0.0f, 0.0f));

    session.undo();
    CHECK(node->transform().position() == Vec3(2.0f, 0.0f, 0.0f));
    session.undo();
    CHECK(node->transform().position() == Vec3(0.0f, 0.0f, 0.0f));

    // Имя - свойство самого узла.
    session.setProperty(*node, property("Node", "name"), std::string("Player"));
    CHECK(node->name() == "Player");
    session.undo();
    CHECK(node->name() == "Node3D");
}

TEST_CASE_FIXTURE(Fixture, "Undo: the manipulator moves a node in one step")
{
    auto* node = static_cast<Node3D*>(session.createNode(type("Node3D"), nullptr));
    session.finishEdit();

    session.setTransform(*node, {.position = {1.0f, 0.0f, 0.0f}});
    session.setTransform(*node, {.position = {2.0f, 0.0f, 0.0f}, .rotationDegrees = {0.0f, 90.0f, 0.0f}});
    CHECK(node->transform().rotationDegrees() == Vec3(0.0f, 90.0f, 0.0f));

    session.undo();
    CHECK(node->transform().position() == Vec3(0.0f));
    CHECK(node->transform().rotationDegrees() == Vec3(0.0f));
}

TEST_CASE_FIXTURE(Fixture, "Undo: history survives Play and Stop")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    Node* child = session.createNode(type("Node3D"), root);
    const NodeId childId = child->id();

    session.play();
    CHECK_FALSE(session.canUndo());
    CHECK_FALSE(session.undo());
    session.stop();

    // После Stop снова редактируемая сцена, история продолжается с того же места.
    REQUIRE(session.undo());
    CHECK(session.findNode(childId) == nullptr);
    REQUIRE(session.redo());
    CHECK(session.findNode(childId));
}

TEST_CASE("Selection: several nodes, the last selected is the main one")
{
    Selection selection;
    selection.select(1);
    selection.toggle(2);
    selection.toggle(3);
    CHECK(selection.selected() == 3);
    CHECK(selection.isSelected(1));
    selection.toggle(3);
    CHECK(selection.selected() == 2);
    selection.select(5);
    CHECK(selection.nodes().size() == 1);
    selection.select(0);
    CHECK(selection.empty());
}

TEST_CASE_FIXTURE(Fixture, "Undo: deleting several nodes is one step and brings the selection back")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    Node* a = session.createNode(type("Node3D"), root);
    Node* inner = session.createNode(type("Node3D"), a);
    Node* b = session.createNode(type("Node3D"), root);
    const NodeId aId = a->id();
    const NodeId innerId = inner->id();
    const NodeId bId = b->id();

    session.selection().select(aId);
    session.selection().toggle(innerId);
    session.selection().toggle(bId);
    const Selection before = session.selection();
    const std::vector<NodeId> nodes(before.nodes().begin(), before.nodes().end());

    // inner лежит в удаляемой ветке a и уходит вместе с ней.
    REQUIRE(session.deleteNodes(nodes));
    CHECK(root->children().empty());
    CHECK(session.selection().empty());

    REQUIRE(session.undo());
    CHECK(root->children().size() == 2);
    CHECK(session.findNode(innerId));
    CHECK(session.selection() == before);
}
