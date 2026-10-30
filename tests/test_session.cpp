#include <doctest/doctest.h>

#include <yqlora/core/EngineServices.hpp>
#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/input/Input.hpp>
#include <yqlora/platform/InputFeeder.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/render/Camera3D.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

// Компонент, которого нет в движке: в движке пока нет создаваемых компонентов.
struct TestComponent final : Component {
    YQLORA_TYPE(TestComponent, Component)
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
    Input input;
    EngineServices services{input, registry};
    EditorSession session{services, serializer};
    Scene& scene = session.scene();
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "EditorSession: the first node becomes the root, the next ones its children")
{
    Node* root = session.editing().createNode(type("Node3D"), nullptr);
    REQUIRE(root);
    CHECK(scene.root() == root);
    CHECK(session.selection().selected() == root->id());
    CHECK(session.document().modified());

    Node* cube = session.editing().createNode(type("Camera3D"), nullptr);
    Node* cube2 = session.editing().createNode(type("Camera3D"), nullptr);
    REQUIRE(cube);
    REQUIRE(cube2);
    CHECK(cube->parent() == root);
    // Имена среди соседей не повторяются.
    CHECK(cube->name() == "Camera3D");
    CHECK(cube2->name() == "Camera3D2");

    Node* child = session.editing().createNode(type("Camera3D"), cube);
    REQUIRE(child);
    CHECK(child->parent() == cube);
    CHECK(session.selectedNode() == child);

    // Transform отдельно не создаётся.
    CHECK(session.editing().createNode(type("Transform3D"), nullptr) == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: deleting a branch resets the selection inside it")
{
    Node* root = session.editing().createNode(type("Node3D"), nullptr);
    Node* holder = session.editing().createNode(type("Node3D"), root);
    auto* camera = static_cast<Camera3D*>(session.editing().createNode(type("Camera3D"), holder));
    session.selection().select(camera->id());
    const NodeId holderId = holder->id();

    REQUIRE(session.editing().deleteNode(holderId));
    CHECK(session.findNode(holderId) == nullptr);
    CHECK(session.selection().selected() == 0);
    CHECK(root->children().empty());

    // Корень тоже можно удалить: сцена становится пустой.
    REQUIRE(session.editing().deleteNode(root->id()));
    CHECK(scene.empty());
    CHECK_FALSE(session.editing().deleteNode(12345));
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: components can be added and removed, Transform stays")
{
    Node* node = session.editing().createNode(type("Node3D"), nullptr);
    Component* component = session.editing().addComponent(*node, type("TestComponent"));
    REQUIRE(component);
    CHECK(node->components().size() == 2);

    CHECK(session.editing().removeComponent(*node, *component));
    CHECK(node->components().size() == 1);
    CHECK_FALSE(session.editing().removeComponent(*node, *node->components()[0]));
    CHECK(session.editing().addComponent(*node, type("Transform3D")) == nullptr);
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: Stop returns the scene as it was before Play")
{
    Node* root = session.editing().createNode(type("Node3D"), nullptr);
    auto* cube = static_cast<Node3D*>(session.editing().createNode(type("Camera3D"), root));
    cube->transform().setPosition({1.0f, 0.0f, 0.0f});
    const NodeId cubeId = cube->id();
    session.selection().select(cubeId);
    const bool modifiedBefore = session.document().modified();

    session.play();
    CHECK(session.playing());
    CHECK(session.playMode().running());
    // Игра идёт в копии: те же ID, но другие объекты.
    CHECK(&session.scene() != &scene);
    auto* playCube = dynamic_cast<Node3D*>(session.findNode(cubeId));
    REQUIRE(playCube);
    CHECK(playCube != cube);
    CHECK(session.selection().selected() == cubeId);

    // Во время игры копию меняют, а изменения не помечаются и не сохраняются.
    playCube->transform().setPosition({5.0f, 5.0f, 5.0f});
    session.editing().createNode(type("Node3D"), session.findNode(root->id()));
    CHECK(cube->transform().position() == Vec3(1.0f, 0.0f, 0.0f));
    CHECK(session.document().modified() == modifiedBefore);
    CHECK_FALSE(session.editing().canUndo());
    CHECK_FALSE(session.saveScene());

    session.setPaused(true);
    CHECK_FALSE(session.playMode().running());

    session.stop();
    CHECK(&session.scene() == &scene);
    CHECK_FALSE(session.playing());
    REQUIRE(scene.root());
    CHECK(scene.root()->children().size() == 1);
    auto* restored = dynamic_cast<Node3D*>(session.findNode(cubeId));
    REQUIRE(restored);
    CHECK(restored->transform().position() == Vec3(1.0f, 0.0f, 0.0f));
    CHECK(session.selection().selected() == cubeId);
    CHECK(session.document().modified() == modifiedBefore);
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: the game sees input only while its window is focused")
{
    session.editing().createNode(type("Node3D"), nullptr);
    Input appInput;
    InputFeeder feeder(appInput);
    feeder.keyDown(Key::Space);
    constexpr float kFrame = 0.016f;

    session.play();
    const Input& gameInput = session.scene().services().input();
    // Клавиши, набранные в инспекторе, до игры не доходят.
    session.update(kFrame, appInput);
    CHECK_FALSE(gameInput.isKeyDown(Key::Space));

    session.setGameInputFocused(true);
    session.update(kFrame, appInput);
    CHECK(gameInput.isKeyDown(Key::Space));

    // Ввод игры свой: редактируемая сцена по-прежнему видит службы редактора.
    CHECK(&gameInput != &scene.services().input());
}
