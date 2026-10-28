#include <doctest/doctest.h>

#include <yqlora/editor/EditorSession.hpp>
#include <yqlora/reflection/ComponentRules.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/MeshRenderer.hpp>
#include <yqlora/scene/Scene.hpp>
#include <yqlora/scene/SceneSerializer.hpp>

#include <algorithm>

using namespace yqlora;
using namespace yqlora::editor;

namespace {

struct Body : Component {
    YQLORA_TYPE(Body, Component)
};

// Тоже тело: закрывает требование Body.
struct HeavyBody final : Body {
    YQLORA_TYPE(HeavyBody, Body)
};

struct Collider : Component {
    YQLORA_TYPE(Collider, Component)
};

// Требования базового Collider действуют и на него.
struct BoxCollider final : Collider {
    YQLORA_TYPE(BoxCollider, Collider)
};

struct Sound final : Component {
    YQLORA_TYPE(Sound, Component)
};

// Цепочка и цикл требований.
struct First final : Component {
    YQLORA_TYPE(First, Component)
};

struct Second final : Component {
    YQLORA_TYPE(Second, Component)
};

struct Third final : Component {
    YQLORA_TYPE(Third, Component)
};

struct Unregistered final : Component {
    YQLORA_TYPE(Unregistered, Component)
};

struct NeedsUnregistered final : Component {
    YQLORA_TYPE(NeedsUnregistered, Component)
};

struct Fixture {
    Fixture()
    {
        registerEngineTypes(registry);
        registry.registerComponent<Body>().requiresNode<Node3D>();
        registry.registerComponent<HeavyBody>();
        registry.registerComponent<Collider>().requiresComponent<Body>();
        registry.registerComponent<BoxCollider>();
        registry.registerComponent<Sound>().allowMultiple();
        registry.registerComponent<First>().requiresComponent<Second>();
        registry.registerComponent<Second>().requiresComponent<Third>();
        registry.registerComponent<Third>().requiresComponent<Second>();
        registry.registerComponent<NeedsUnregistered>().requiresComponent<Unregistered>();
    }

    const TypeInfo& type(std::string_view name) const
    {
        return *registry.find(name);
    }

    std::vector<std::string> names(const std::vector<const TypeInfo*>& types) const
    {
        std::vector<std::string> result;
        for (const TypeInfo* t : types) {
            result.push_back(t->name());
        }

        return result;
    }

    TypeRegistry registry;
};

} // namespace

TEST_CASE_FIXTURE(Fixture, "ComponentRules: requiresNode accepts the type and its descendants only")
{
    const Node plain;
    const Node3D node3D;
    const Camera3D camera;

    CHECK_FALSE(canAttach(registry, type("MeshRenderer"), plain));
    CHECK(canAttach(registry, type("MeshRenderer"), node3D));
    CHECK(canAttach(registry, type("MeshRenderer"), camera));

    // Правило базового Body действует и на HeavyBody.
    CHECK_FALSE(canAttach(registry, type("HeavyBody"), plain));
    CHECK(canAttach(registry, type("HeavyBody"), node3D));
}

TEST_CASE_FIXTURE(Fixture, "ComponentRules: one component of a type unless allowMultiple")
{
    Node3D node;
    node.addComponent<MeshRenderer>();
    node.addComponent<Sound>();

    CHECK_FALSE(canAttach(registry, type("MeshRenderer"), node));
    CHECK(canAttach(registry, type("Sound"), node));
}

TEST_CASE_FIXTURE(Fixture, "ComponentRules: missing requirements come in the order to add them")
{
    Node3D node;
    REQUIRE(missingRequirements(registry, type("Collider"), node));
    CHECK(names(*missingRequirements(registry, type("Collider"), node)) == std::vector<std::string>{"Body"});

    // Требование унаследовано от Collider.
    CHECK(names(*missingRequirements(registry, type("BoxCollider"), node)) == std::vector<std::string>{"Body"});

    // Наследник требуемого типа тоже подходит.
    node.addComponent<HeavyBody>();
    CHECK(missingRequirements(registry, type("Collider"), node)->empty());
}

TEST_CASE_FIXTURE(Fixture, "ComponentRules: requirements of requirements, and cycles end")
{
    const Node3D node;
    // First -> Second -> Third -> Second: Second уже решено добавить, цикл обрывается.
    CHECK(names(*missingRequirements(registry, type("First"), node)) == std::vector<std::string>{"Third", "Second"});
}

TEST_CASE_FIXTURE(Fixture, "ComponentRules: a requirement that cannot be met")
{
    const Node plain;
    const Node3D node;
    // Тип требования не зарегистрирован.
    CHECK_FALSE(missingRequirements(registry, type("NeedsUnregistered"), node));
    // Body не живёт на голом Node, значит и Collider на нём не собрать.
    CHECK_FALSE(missingRequirements(registry, type("Collider"), plain));
}

TEST_CASE_FIXTURE(Fixture, "ComponentRules: dependents of a component")
{
    Node3D node;
    auto& body = node.addComponent<Body>();
    auto& collider = node.addComponent<BoxCollider>();

    CHECK(dependentsOf(registry, node, body) == std::vector<const Component*>{&collider});
    CHECK(dependentsOf(registry, node, collider).empty());

    // Есть другое тело: без первого коллайдер не останется.
    node.addComponent<HeavyBody>();
    CHECK(dependentsOf(registry, node, body).empty());
}

TEST_CASE_FIXTURE(Fixture, "EditorSession: component rules")
{
    SceneSerializer serializer{registry};
    Scene scene;
    EditorSession session{scene, serializer};
    Node* node = session.createNode(type("Node3D"), nullptr);
    REQUIRE(node);

    const auto offers = [&](std::string_view name) {
        return std::ranges::any_of(session.addableComponents(*node), [&](const TypeInfo* t) {
            return t->name() == name;
        });
    };
    CHECK(offers("MeshRenderer"));
    CHECK_FALSE(offers("NeedsUnregistered"));
    // Встроенный Transform3D не создаётся.
    CHECK_FALSE(offers("Transform3D"));

    SUBCASE("a required component is added in the same history step")
    {
        Component* collider = session.addComponent(*node, type("BoxCollider"));
        REQUIRE(collider);
        REQUIRE(node->components().size() == 3);
        CHECK(node->components()[1]->typeName() == "Body");
        CHECK(node->components()[2].get() == collider);

        // Тело нужно коллайдеру: снять нельзя.
        Component& body = *node->components()[1];
        CHECK(session.componentDependents(*node, body) == std::vector<const Component*>{collider});
        CHECK_FALSE(session.removeComponent(*node, body));
        CHECK(node->components().size() == 3);

        REQUIRE(session.undo());
        CHECK(node->components().size() == 1);
        REQUIRE(session.redo());
        CHECK(node->components().size() == 3);
    }

    SUBCASE("no second component of a type and none that does not fit")
    {
        REQUIRE(session.addComponent(*node, type("MeshRenderer")));
        CHECK_FALSE(offers("MeshRenderer"));
        CHECK_FALSE(session.addComponent(*node, type("MeshRenderer")));

        Node* plain = session.createNode(type("Node"), node);
        REQUIRE(plain);
        CHECK_FALSE(session.addComponent(*plain, type("MeshRenderer")));
        CHECK(plain->components().empty());
    }
}

TEST_CASE_FIXTURE(Fixture, "SceneSerializer: component rules on load")
{
    const SceneSerializer serializer{registry};

    Scene source;
    auto& root = source.createRoot<Node3D>("Root");
    root.addComponent<BoxCollider>();                          // тела нет: файл как будто правили руками
    root.addChild<Node>("Plain").addComponent<MeshRenderer>(); // не тот узел

    Scene loaded;
    REQUIRE(serializer.fromJson(loaded, serializer.toJson(source)));
    const Node* loadedRoot = loaded.root();
    REQUIRE(loadedRoot);

    // Обязательное тело добавлено.
    REQUIRE(loadedRoot->components().size() == 3);
    CHECK(loadedRoot->components()[1]->typeName() == "BoxCollider");
    CHECK(loadedRoot->components()[2]->typeName() == "Body");

    // Компонент на неподходящем узле пропущен.
    REQUIRE(loadedRoot->children().size() == 1);
    CHECK(loadedRoot->children()[0]->components().empty());
}
