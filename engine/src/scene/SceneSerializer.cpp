#include "yqlora/scene/SceneSerializer.hpp"

#include "yqlora/core/FileFormat.hpp"
#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/JsonId.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/reflection/ComponentRules.hpp"
#include "yqlora/reflection/PropertyJson.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/NodeTree.hpp"
#include "yqlora/scene/Scene.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <optional>

namespace yqlora {

namespace {

using Json = nlohmann::json;

// Переводы файла сцены со старых версий, см. FileFormat. Пока формат один, переводов нет.
constexpr int kFirstFormatVersion = 1;
constexpr std::array<Migration, 0> kMigrations{};
constexpr FileFormat kFormat{"Scene", kFirstFormatVersion, kMigrations};

// Ключи текущего формата.
constexpr const char* kRootKey = "root";
constexpr const char* kClearColorKey = "clearColor";
constexpr const char* kActiveCameraKey = "activeCamera";
constexpr const char* kTypeKey = "type";
constexpr const char* kIdKey = "id";
constexpr const char* kPropertiesKey = "properties";
constexpr const char* kComponentsKey = "components";
constexpr const char* kChildrenKey = "children";

Json nodeToJson(const TypeRegistry& registry, const Node& node)
{
    const TypeInfo* type = registry.find(node.typeName());
    if (!type) {
        log::error("Scene: node type '{}' is not registered, node '{}' is not saved", node.typeName(), node.name());

        return nullptr;
    }

    Json result;
    result[kTypeKey] = type->name();
    result[kIdKey] = idToJson(node.id());
    result[kPropertiesKey] = propertiesToJson(registry, *type, node);

    Json components = Json::array();
    for (const auto& component : node.components()) {
        const TypeInfo* componentType = registry.find(component->typeName());
        if (!componentType) {
            log::error("Scene: component type '{}' is not registered, not saved", component->typeName());
            continue;
        }
        components.push_back({
            {kTypeKey, componentType->name()},
            {kPropertiesKey, propertiesToJson(registry, *componentType, *component)},
        });
    }
    result[kComponentsKey] = std::move(components);

    Json children = Json::array();
    for (const auto& child : node.children()) {
        Json childJson = nodeToJson(registry, *child);
        if (!childJson.is_null()) {
            children.push_back(std::move(childJson));
        }
    }
    result[kChildrenKey] = std::move(children);

    return result;
}

} // namespace

// Чтение и запись сцены. Вложен в SceneSerializer, чтобы вызывать закрытый assignId.
struct SceneSerializer::Format {
    const TypeRegistry& registry;

    Json write(const Scene& scene) const;
    bool readScene(Scene& scene, Json json) const;

    std::unique_ptr<Node> readNode(const Json& json) const
    {
        if (!json.is_object() || !json.contains(kTypeKey) || !json[kTypeKey].is_string()) {
            log::error("Scene: node without a type");

            return nullptr;
        }
        const std::string typeName = json[kTypeKey].get<std::string>();
        const TypeInfo* type = registry.find(typeName);
        if (!type || type->kind() != TypeKind::Node) {
            log::error("Scene: unknown node type '{}'", typeName);

            return nullptr;
        }
        std::unique_ptr<Node> node = type->createNode();
        if (!node) {
            log::error("Scene: node type '{}' cannot be created", typeName);

            return nullptr;
        }

        const auto id = json.contains(kIdKey) ? idFromJson(json[kIdKey]) : std::nullopt;
        if (id && *id != 0) {
            assignId(*node, *id);
        }
        if (const auto it = json.find(kPropertiesKey); it != json.end()) {
            propertiesFromJson(registry, *type, *node, *it);
        }
        if (const auto it = json.find(kComponentsKey); it != json.end() && it->is_array()) {
            for (const Json& componentJson : *it) {
                readComponent(*node, componentJson);
            }
        }
        addMissingRequirements(*node);
        if (const auto it = json.find(kChildrenKey); it != json.end() && it->is_array()) {
            for (const Json& childJson : *it) {
                std::unique_ptr<Node> child = readNode(childJson);
                if (!child) {
                    return nullptr;
                }
                node->addChild(std::move(child));
            }
        }

        return node;
    }

    // Неизвестный компонент пропускаем с предупреждением, а не роняем всю сцену:
    // так сцена откроется, даже если компонент удалили из кода игры.
    void readComponent(Node& node, const Json& json) const
    {
        if (!json.is_object() || !json.contains(kTypeKey) || !json[kTypeKey].is_string()) {
            log::warn("Scene: component without a type on node '{}', skipped", node.name());

            return;
        }
        const std::string typeName = json[kTypeKey].get<std::string>();
        const TypeInfo* type = registry.find(typeName);
        if (!type || type->kind() != TypeKind::Component) {
            log::warn("Scene: unknown component type '{}' on node '{}', skipped", typeName, node.name());

            return;
        }

        Component* component = nullptr;
        if (type->creatable()) {
            if (!canAttach(registry, *type, node)) {
                log::error(
                    "Scene: component '{}' does not fit node '{}' of type '{}', skipped",
                    typeName,
                    node.name(),
                    node.typeName()
                );

                return;
            }
            component = &node.addComponent(type->createComponent());
        } else {
            // Встроенный компонент узел уже создал сам, только заполняем его.
            for (const auto& existing : node.components()) {
                if (existing->typeName() == typeName) {
                    component = existing.get();
                    break;
                }
            }
        }
        if (!component) {
            log::warn("Scene: component '{}' cannot be added to node '{}', skipped", typeName, node.name());

            return;
        }
        if (const auto it = json.find(kPropertiesKey); it != json.end()) {
            propertiesFromJson(registry, *type, *component, *it);
        }
    }

    // Файл правили руками или правила типа поменялись: обязательного компонента
    // в файле нет. Добавляем его со значениями по умолчанию, чтобы правила
    // выполнялись для любой загруженной сцены.
    void addMissingRequirements(Node& node) const
    {
        std::vector<const TypeInfo*> present;
        for (const auto& component : node.components()) {
            if (const TypeInfo* type = registry.find(component->typeName())) {
                present.push_back(type);
            }
        }
        for (const TypeInfo* type : present) {
            const auto missing = missingRequirements(registry, *type, node);
            if (!missing) {
                log::error(
                    "Scene: requirements of component '{}' on node '{}' cannot be met",
                    type->name(),
                    node.name()
                );
                continue;
            }
            for (const TypeInfo* required : *missing) {
                log::warn(
                    "Scene: component '{}' on node '{}' requires '{}', added",
                    type->name(),
                    node.name(),
                    required->name()
                );
                node.addComponent(required->createComponent());
            }
        }
    }
};

SceneSerializer::SceneSerializer(const TypeRegistry& registry)
    : m_registry(registry)
{
}

void SceneSerializer::assignId(Node& node, NodeId id)
{
    node.setId(id);
}

Json SceneSerializer::Format::write(const Scene& scene) const
{
    Json json;
    kFormat.stamp(json);
    json[kClearColorKey] = vectorToJson(scene.clearColor());
    json[kActiveCameraKey] = idToJson(scene.activeCamera() ? scene.activeCamera()->id() : NodeId{0});
    json[kRootKey] = scene.root() ? nodeToJson(registry, *scene.root()) : Json(nullptr);

    return json;
}

bool SceneSerializer::Format::readScene(Scene& scene, Json json) const
{
    if (!json.is_object()) {
        log::error("Scene: file is not a valid JSON object");

        return false;
    }
    if (!kFormat.migrate(json)) {
        return false;
    }

    // Дерево собираем отдельно и отдаём сцене, только если всё прочиталось.
    std::unique_ptr<Node> root;
    if (const auto it = json.find(kRootKey); it != json.end() && !it->is_null()) {
        root = readNode(*it);
        if (!root) {
            return false;
        }
    }

    scene.setRoot(std::move(root));
    if (const auto it = json.find(kClearColorKey); it != json.end()) {
        if (const auto color = vectorFromJson<Vec4>(*it)) {
            scene.setClearColor(*color);
        }
    }
    const auto camera = json.contains(kActiveCameraKey) ? idFromJson(json[kActiveCameraKey]) : std::nullopt;
    if (camera && scene.root()) {
        Node* node = findNode(*scene.root(), *camera);
        scene.setActiveCamera(dynamic_cast<Camera3D*>(node));
    }

    return true;
}

std::string SceneSerializer::toJson(const Scene& scene) const
{
    return Format{m_registry}.write(scene).dump(kJsonIndent);
}

bool SceneSerializer::fromJson(Scene& scene, std::string_view text) const
{
    Json json = Json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (json.is_discarded()) {
        log::error("Scene: file is not a valid JSON object");

        return false;
    }

    return Format{m_registry}.readScene(scene, std::move(json));
}

bool SceneSerializer::save(const Scene& scene, const std::filesystem::path& file) const
{
    if (!writeJsonFile(file, Format{m_registry}.write(scene))) {
        log::error("Scene: cannot write '{}'", toUtf8(file));

        return false;
    }

    return true;
}

bool SceneSerializer::load(Scene& scene, const std::filesystem::path& file) const
{
    auto json = std::filesystem::exists(file) ? readJsonFile(file) : std::nullopt;
    if (!json) {
        log::error("Scene: cannot open '{}'", toUtf8(file));

        return false;
    }

    return Format{m_registry}.readScene(scene, std::move(*json));
}

} // namespace yqlora
