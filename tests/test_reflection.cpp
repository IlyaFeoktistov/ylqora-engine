#include <doctest/doctest.h>

#include <yqlora/i18n/Localization.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node3D.hpp>

#include <string>
#include <vector>

using namespace yqlora;

namespace {

// Компонент, которого нет в движке: так тесты проверяют типы, зарегистрированные снаружи.
struct TestComponent final : Component {
    std::string_view typeName() const override
    {
        return "TestComponent";
    }

    int value = 100;
};

std::vector<std::string> names(const std::vector<const Property*>& properties)
{
    std::vector<std::string> result;
    for (const Property* property : properties) {
        result.push_back(property->name());
    }

    return result;
}

} // namespace

TEST_CASE("TypeRegistry: engine types form an inheritance tree")
{
    TypeRegistry registry;
    registerEngineTypes(registry);

    const TypeInfo* node = registry.find("Node");
    const TypeInfo* node3D = registry.find("Node3D");
    const TypeInfo* camera = registry.find("Camera3D");
    const TypeInfo* transform = registry.find("Transform3D");
    REQUIRE(node);
    REQUIRE(node3D);
    REQUIRE(camera);
    REQUIRE(transform);

    CHECK(node->baseName().empty());
    CHECK(node3D->baseName() == "Node");
    CHECK(camera->baseName() == "Node3D");
    CHECK(camera->kind() == TypeKind::Node);
    CHECK(transform->kind() == TypeKind::Component);
    CHECK(registry.find<Camera3D>() == camera);
    CHECK(registry.find("Missing") == nullptr);
}

TEST_CASE("TypeRegistry: nodes are created by type name, Transform is not")
{
    TypeRegistry registry;
    registerEngineTypes(registry);

    auto camera = registry.find("Camera3D")->createNode();
    REQUIRE(camera);
    CHECK(camera->typeName() == "Camera3D");

    // Transform создаёт сам узел, из меню или файла его не добавить.
    const TypeInfo* transform = registry.find("Transform3D");
    CHECK_FALSE(transform->creatable());
    CHECK(transform->createComponent() == nullptr);
}

TEST_CASE("TypeRegistry: properties include base types, base first")
{
    TypeRegistry registry;
    registerEngineTypes(registry);

    const auto properties = registry.properties(*registry.find("Camera3D"));
    CHECK(names(properties) == std::vector<std::string>{"name", "fov", "near", "far"});
}

TEST_CASE("Property: reads and writes values through getters and setters")
{
    TypeRegistry registry;
    registerEngineTypes(registry);

    Camera3D camera("Main");
    const auto properties = registry.properties(*registry.find("Camera3D"));
    const Property& name = *properties[0];
    const Property& fov = *properties[1];

    CHECK(std::get<std::string>(name.get(camera)) == "Main");
    CHECK(fov.type() == PropertyType::Float);
    CHECK(fov.set(camera, 75.0f));
    CHECK(camera.fovDegrees() == doctest::Approx(75.0f));

    // Значение другого типа не применяется.
    CHECK_FALSE(fov.set(camera, std::string("wide")));
    CHECK(camera.fovDegrees() == doctest::Approx(75.0f));

    const Property& position = *registry.properties(*registry.find("Transform3D"))[0];
    CHECK(position.type() == PropertyType::Vector3);
    CHECK(position.set(camera.transform(), Vec3(1.0f, 2.0f, 3.0f)));
    CHECK(camera.transform().position() == Vec3(1.0f, 2.0f, 3.0f));
}

TEST_CASE("TypeRegistry: components registered outside the engine expose fields")
{
    TypeRegistry registry;
    registerEngineTypes(registry);
    registry.registerComponent<TestComponent>("TestComponent").property("value", &TestComponent::value);

    const TypeInfo* type = registry.find("TestComponent");
    REQUIRE(type);
    CHECK(type->creatable());

    Node3D node;
    auto& component = static_cast<TestComponent&>(node.addComponent(type->createComponent()));
    const Property& value = *type->ownProperties().data();
    CHECK(value.type() == PropertyType::Int);
    CHECK(value.set(component, 42));
    CHECK(component.value == 42);
    CHECK(std::get<int>(value.get(component)) == 42);
}

TEST_CASE("TypeRegistry: every engine type has a description in every language")
{
    TypeRegistry registry;
    registerEngineTypes(registry);

    Localization en(YQLORA_LOCALES_DIR);
    REQUIRE(en.load("en"));
    Localization ru(YQLORA_LOCALES_DIR);
    REQUIRE(ru.load("ru"));

    for (const auto& type : registry.types()) {
        const std::string key = type->descriptionKey();
        CAPTURE(key);
        CHECK(en.translate(key) != key);
        // Без своего перевода ru показал бы английский текст.
        CHECK(ru.translate(key) != en.translate(key));
    }
}
