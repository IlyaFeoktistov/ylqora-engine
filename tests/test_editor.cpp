#include <doctest/doctest.h>

#include <yqlora/editor/EditorCamera.hpp>
#include <yqlora/editor/Picking.hpp>
#include <yqlora/editor/Selection.hpp>
#include <yqlora/platform/InputFeeder.hpp>
#include <yqlora/scene/MeshInstance3D.hpp>
#include <yqlora/scene/Scene.hpp>

#include <glm/gtc/epsilon.hpp>

using namespace yqlora;

namespace {

bool near(const Vec3& a, const Vec3& b)
{
    return glm::all(glm::epsilonEqual(a, b, 1e-4f));
}

} // namespace

TEST_CASE("EditorCamera: moves only while the right mouse button is held")
{
    EditorCamera camera;
    camera.setPosition({0.0f, 0.0f, 0.0f});
    camera.lookAt({0.0f, 0.0f, -1.0f});
    camera.setSpeed(2.0f);

    Input input;
    InputFeeder feeder(input);
    feeder.beginFrame();
    feeder.keyDown(Key::W);

    camera.update(input, 1.0f, true);
    CHECK_FALSE(camera.flying());
    CHECK(near(camera.position(), {0.0f, 0.0f, 0.0f}));

    feeder.mouseButtonDown(MouseButton::Right);
    camera.update(input, 1.0f, true);
    CHECK(camera.flying());
    // Вперёд вдоль -Z со скоростью 2 метра в секунду.
    CHECK(near(camera.position(), {0.0f, 0.0f, -2.0f}));

    // Курсор ушёл с окна сцены, а кнопка всё ещё зажата: полёт продолжается.
    feeder.beginFrame();
    camera.update(input, 1.0f, false);
    CHECK(camera.flying());
}

TEST_CASE("EditorCamera: right button pressed outside the viewport does not start flying")
{
    EditorCamera camera;

    Input input;
    InputFeeder feeder(input);
    feeder.beginFrame();
    feeder.mouseButtonDown(MouseButton::Right);
    camera.update(input, 0.016f, false);
    CHECK_FALSE(camera.flying());

    // Кнопку так и не отпустили, но курсор зашёл на окно сцены: полёт не начинается.
    feeder.beginFrame();
    camera.update(input, 0.016f, true);
    CHECK_FALSE(camera.flying());
}

TEST_CASE("EditorCamera: wheel changes speed within limits")
{
    EditorCamera camera;
    camera.setSpeed(5.0f);

    Input input;
    InputFeeder feeder(input);
    feeder.beginFrame();
    feeder.wheelScrolled(1.0f);
    camera.update(input, 0.016f, true);
    CHECK(camera.speed() > 5.0f);

    // Колесо над другой панелью камеру не трогает.
    const float speed = camera.speed();
    feeder.beginFrame();
    feeder.wheelScrolled(1.0f);
    camera.update(input, 0.016f, false);
    CHECK(camera.speed() == doctest::Approx(speed));

    feeder.beginFrame();
    feeder.wheelScrolled(-1000.0f);
    camera.update(input, 0.016f, true);
    CHECK(camera.speed() == doctest::Approx(0.1f));
}

TEST_CASE("EditorCamera: ray through the screen center goes forward")
{
    EditorCamera camera;
    camera.setPosition({0.0f, 2.0f, 5.0f});
    camera.lookAt({0.0f, 2.0f, 0.0f});

    const Ray ray = camera.screenRay({640.0f, 360.0f}, {1280.0f, 720.0f});
    CHECK(near(ray.origin, {0.0f, 2.0f, 5.0f}));
    CHECK(near(ray.direction, {0.0f, 0.0f, -1.0f}));

    // Правый край экрана - луч уходит вправо.
    CHECK(camera.screenRay({1280.0f, 360.0f}, {1280.0f, 720.0f}).direction.x > 0.0f);
}

TEST_CASE("Picking: the nearest mesh under the ray wins")
{
    Scene scene;
    auto& root = scene.createRoot<Node>();
    auto& front = root.addChild<MeshInstance3D>("Front");
    front.transform().setPosition({0.0f, 0.0f, -3.0f});
    auto& back = root.addChild<MeshInstance3D>("Back");
    back.transform().setPosition({0.0f, 0.0f, -6.0f});
    auto& ball = root.addChild<MeshInstance3D>("Ball");
    ball.setMesh("sphere");
    ball.transform().setPosition({3.0f, 0.0f, -3.0f});

    const Vec3 origin(0.0f, 0.0f, 0.0f);
    CHECK(pickNode(scene, {origin, {0.0f, 0.0f, -1.0f}}) == front.id());
    CHECK(pickNode(scene, {Vec3(3.0f, 0.0f, 0.0f), {0.0f, 0.0f, -1.0f}}) == ball.id());
    // Угол вокруг сферы пустой: попадание считается по треугольникам, а не по габаритам.
    CHECK(pickNode(scene, {Vec3(3.45f, 0.45f, 0.0f), {0.0f, 0.0f, -1.0f}}) == 0);
    CHECK(pickNode(scene, {origin, {0.0f, 1.0f, 0.0f}}) == 0);

    // Скрытый за передним куб находится, если передний убрать с пути.
    front.transform().setPosition({0.0f, 10.0f, -3.0f});
    CHECK(pickNode(scene, {origin, {0.0f, 0.0f, -1.0f}}) == back.id());
}

TEST_CASE("Picking: scale and rotation of the node are taken into account")
{
    Scene scene;
    auto& cube = scene.createRoot<MeshInstance3D>();
    cube.transform().setPosition({0.0f, 0.0f, -5.0f});
    cube.transform().setScale({4.0f, 1.0f, 1.0f});

    // Без масштаба куб шириной 1 сюда бы не дотянулся.
    CHECK(pickNode(scene, {Vec3(1.5f, 0.0f, 0.0f), {0.0f, 0.0f, -1.0f}}) == cube.id());
    CHECK(pickNode(Scene{}, {Vec3(0.0f), {0.0f, 0.0f, -1.0f}}) == 0);
}

TEST_CASE("Selection: one node at a time, 0 means nothing")
{
    Selection selection;
    CHECK(selection.selected() == 0);
    CHECK_FALSE(selection.isSelected(0));

    selection.select(42);
    CHECK(selection.isSelected(42));

    selection.clear();
    CHECK(selection.selected() == 0);
}
