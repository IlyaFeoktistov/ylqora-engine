#include <doctest/doctest.h>

#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

// Компонент, которого нет в движке: в движке пока нет создаваемых компонентов.
struct TestComponent final : Component {
    YQLORA_TYPE(TestComponent)
};

struct Fixture {
    Fixture()
    {
        registerEngineTypes(registry);
        registry.registerComponent<TestComponent>();
    }

    const TypeInfo& type(std::string_view name) const
    {
        return *registry.find(name);
    }

    TypeRegistry registry;
    SceneSerializer serializer{registry};
    Scene scene;
    EditorSession session{scene, serializer};
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "EditorSession: the first node becomes the root, the next ones its children")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    REQUIRE(root);
    CHECK(scene.root() == root);
    CHECK(session.selection().selected() == root->id());
    CHECK(session.modified());

    Node* cube = session.createNode(type("Camera3D"), nullptr);
    Node* cube2 = session.createNode(type("Camera3D"), nullptr);
    REQUIRE(cube);
    REQUIRE(cube2);
    CHECK(cube->parent() == root);
    // Имена среди соседей не повторяются.
    CHECK(cube->name() == "Camera3D");
    CHECK(cube2->name() == "Camera3D2");

    Node* child = session.createNode(type("Camera3D"), cube);
    REQUIRE(child);
    CHECK(child->parent() == cube);
    CHECK(session.selectedNode() == child);

    // Transform отдельно не создаётся.
    CHECK(session.createNode(type("Transform3D"), nullptr) == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: deleting a branch resets the active camera and the selection inside it")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    Node* holder = session.createNode(type("Node3D"), root);
    auto* camera = static_cast<Camera3D*>(session.createNode(type("Camera3D"), holder));
    scene.setActiveCamera(camera);
    session.selection().select(camera->id());
    const NodeId holderId = holder->id();

    REQUIRE(session.deleteNode(holderId));
    CHECK(session.findNode(holderId) == nullptr);
    CHECK(scene.activeCamera() == nullptr);
    CHECK(session.selection().selected() == 0);
    CHECK(root->children().empty());

    // Корень тоже можно удалить: сцена становится пустой.
    REQUIRE(session.deleteNode(root->id()));
    CHECK(scene.empty());
    CHECK_FALSE(session.deleteNode(12345));
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: components can be added and removed, Transform stays")
{
    Node* node = session.createNode(type("Node3D"), nullptr);
    Component* component = session.addComponent(*node, type("TestComponent"));
    REQUIRE(component);
    CHECK(node->components().size() == 2);

    CHECK(session.removeComponent(*node, *component));
    CHECK(node->components().size() == 1);
    CHECK_FALSE(session.removeComponent(*node, *node->components()[0]));
    CHECK(session.addComponent(*node, type("Transform3D")) == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: Stop returns the scene as it was before Play")
{
    Node* root = session.createNode(type("Node3D"), nullptr);
    auto* cube = static_cast<Node3D*>(session.createNode(type("Camera3D"), root));
    cube->transform().setPosition({1.0f, 0.0f, 0.0f});
    const NodeId cubeId = cube->id();
    session.selection().select(cubeId);
    const bool modifiedBefore = session.modified();

    session.play();
    CHECK(session.playing());
    CHECK(session.running());

    // Во время игры сцену меняют, а изменения не помечаются и не сохраняются.
    cube->transform().setPosition({5.0f, 5.0f, 5.0f});
    session.createNode(type("Node3D"), root);
    CHECK(session.modified() == modifiedBefore);
    CHECK_FALSE(session.canUndo());
    CHECK_FALSE(session.saveScene());

    session.setPaused(true);
    CHECK_FALSE(session.running());

    session.stop();
    CHECK_FALSE(session.playing());
    REQUIRE(scene.root());
    CHECK(scene.root()->children().size() == 1);
    auto* restored = dynamic_cast<Node3D*>(session.findNode(cubeId));
    REQUIRE(restored);
    CHECK(restored->transform().position() == Vec3(1.0f, 0.0f, 0.0f));
    CHECK(session.selection().selected() == cubeId);
    CHECK(session.modified() == modifiedBefore);
}
