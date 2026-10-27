#include "yqlora/scene/SceneSerializer.hpp"

#include "core/JsonId.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <optional>
#include <sstream>

namespace yqlora {

namespace {

using Json = nlohmann::json;

// Версия формата файла сцены. Когда формат меняется, версия растёт, а в
// kMigrations добавляется перевод с предыдущей версии: старые сцены открываются.
constexpr int kFormatVersion = 2;

template <typename Vec>
Json vectorToJson(const Vec& vec)
{
    Json result = Json::array();
    for (glm::length_t i = 0; i < Vec::length(); ++i) {
        result.push_back(vec[i]);
    }

    return result;
}

template <typename Vec>
std::optional<Vec> vectorFromJson(const Json& json)
{
    if (!json.is_array() || json.size() != static_cast<std::size_t>(Vec::length())) {
        return std::nullopt;
    }
    Vec result;
    for (glm::length_t i = 0; i < Vec::length(); ++i) {
        const Json& item = json[static_cast<std::size_t>(i)];
        if (!item.is_number()) {
            return std::nullopt;
        }
        result[i] = item.get<float>();
    }

    return result;
}

Json valueToJson(const Property& property, const PropertyValue& value)
{
    return std::visit(
        [&](const auto& v) -> Json {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4>) {
                return vectorToJson(v);
            } else if constexpr (std::is_same_v<T, EnumValue>) {
                // Именем, чтобы порядок значений в C++ можно было менять. Без имени - числом.
                const auto& names = property.enumNames();
                const auto index = static_cast<std::size_t>(v.value);
                if (v.value >= 0 && index < names.size()) {
                    return names[index];
                }

                return v.value;
            } else if constexpr (std::is_same_v<T, NodeRef> || std::is_same_v<T, ResourceRef>) {
                return idToJson(v.id);
            } else {
                return Json(v);
            }
        },
        value
    );
}

std::optional<PropertyValue> enumFromJson(const Json& json, const Property& property)
{
    if (json.is_number_integer()) {
        return EnumValue{json.get<int>()};
    }
    if (!json.is_string()) {
        return std::nullopt;
    }
    const auto& names = property.enumNames();
    const auto it = std::ranges::find(names, json.get<std::string>());
    if (it == names.end()) {
        return std::nullopt;
    }

    return EnumValue{static_cast<int>(it - names.begin())};
}

std::optional<PropertyValue> valueFromJson(const Json& json, const Property& property)
{
    switch (property.type()) {
    case PropertyType::Bool:
        if (json.is_boolean()) {
            return json.get<bool>();
        }
        break;
    case PropertyType::Int:
        if (json.is_number_integer()) {
            return json.get<int>();
        }
        break;
    case PropertyType::Float:
        if (json.is_number()) {
            return json.get<float>();
        }
        break;
    case PropertyType::String:
        if (json.is_string()) {
            return json.get<std::string>();
        }
        break;
    case PropertyType::Vector2: return vectorFromJson<Vec2>(json);
    case PropertyType::Vector3: return vectorFromJson<Vec3>(json);
    case PropertyType::Vector4: return vectorFromJson<Vec4>(json);
    case PropertyType::Enum: return enumFromJson(json, property);
    case PropertyType::Node:
        if (const auto id = idFromJson(json)) {
            return NodeRef{*id};
        }
        break;
    case PropertyType::Resource:
        if (const auto id = idFromJson(json)) {
            return ResourceRef{*id};
        }
        break;
    }

    return std::nullopt;
}

// 1 -> 2: узел MeshInstance3D стал Node3D с компонентом MeshRenderer,
// а меш задаётся ID ресурса вместо имени встроенного меша. ID стали строками,
// но числа из версии 1 читаются и так (см. idFromJson).
void migrateMeshInstances(Json& node)
{
    if (!node.is_object()) {
        return;
    }
    if (node.value("type", "") == "MeshInstance3D") {
        node["type"] = "Node3D";
        Json& properties = node["properties"];
        Json renderer = Json::object();
        if (properties.is_object()) {
            if (const auto it = properties.find("mesh"); it != properties.end() && it->is_string()) {
                const auto meshes = builtinMeshes();
                const auto builtin = std::ranges::find(meshes, it->get<std::string>(), &BuiltinMesh::name);
                renderer["mesh"] = idToJson(builtin != meshes.end() ? builtin->id : ResourceId{0});
                properties.erase(it);
            }
            if (const auto it = properties.find("color"); it != properties.end()) {
                renderer["color"] = *it;
                properties.erase(it);
            }
        }
        node["components"].push_back({{"type", "MeshRenderer"}, {"properties", std::move(renderer)}});
    }
    if (const auto it = node.find("children"); it != node.end() && it->is_array()) {
        for (Json& child : *it) {
            migrateMeshInstances(child);
        }
    }
}

void migrate1To2(Json& scene)
{
    if (const auto it = scene.find("root"); it != scene.end()) {
        migrateMeshInstances(*it);
    }
}

using Migration = void (*)(Json& scene);

// kMigrations[i] переводит версию i + 1 в i + 2.
constexpr Migration kMigrations[] = {&migrate1To2};
static_assert(
    std::size(kMigrations) == kFormatVersion - 1,
    "Every format version needs a migration from the previous one"
);

// Приводит файл к текущей версии. false, если версия неизвестна.
bool migrate(Json& scene)
{
    const auto it = scene.find("version");
    if (it == scene.end() || !it->is_number_integer()) {
        log::error("Scene: no format version");

        return false;
    }
    const int version = it->get<int>();
    if (version < 1 || version > kFormatVersion) {
        log::error("Scene: format version {} is not supported, this engine reads up to {}", version, kFormatVersion);

        return false;
    }
    for (int from = version; from < kFormatVersion; ++from) {
        kMigrations[from - 1](scene);
    }
    scene["version"] = kFormatVersion;

    return true;
}

template <typename Object>
Json propertiesToJson(const TypeRegistry& registry, const TypeInfo& type, const Object& object)
{
    Json result = Json::object();
    for (const Property* property : registry.properties(type)) {
        result[property->name()] = valueToJson(*property, property->get(object));
    }

    return result;
}

// Отсутствующие свойства остаются значениями по умолчанию, лишние игнорируются:
// так старые файлы читаются после добавления или удаления свойств.
template <typename Object>
void propertiesFromJson(const TypeRegistry& registry, const TypeInfo& type, Object& object, const Json& json)
{
    if (!json.is_object()) {
        return;
    }
    for (const Property* property : registry.properties(type)) {
        const auto it = json.find(property->name());
        if (it == json.end()) {
            continue;
        }
        const auto value = valueFromJson(*it, *property);
        if (!value) {
            log::warn("Scene: property '{}.{}' has a wrong value, skipped", type.name(), property->name());
            continue;
        }
        property->set(object, *value);
    }
}

Json nodeToJson(const TypeRegistry& registry, const Node& node)
{
    const TypeInfo* type = registry.find(node.typeName());
    if (!type) {
        log::error("Scene: node type '{}' is not registered, node '{}' is not saved", node.typeName(), node.name());

        return nullptr;
    }

    Json result;
    result["type"] = type->name();
    result["id"] = idToJson(node.id());
    result["properties"] = propertiesToJson(registry, *type, node);

    Json components = Json::array();
    for (const auto& component : node.components()) {
        const TypeInfo* componentType = registry.find(component->typeName());
        if (!componentType) {
            log::error("Scene: component type '{}' is not registered, not saved", component->typeName());
            continue;
        }
        components.push_back({
            {"type", componentType->name()},
            {"properties", propertiesToJson(registry, *componentType, *component)},
        });
    }
    result["components"] = std::move(components);

    Json children = Json::array();
    for (const auto& child : node.children()) {
        Json childJson = nodeToJson(registry, *child);
        if (!childJson.is_null()) {
            children.push_back(std::move(childJson));
        }
    }
    result["children"] = std::move(children);

    return result;
}

Node* findById(Node& node, NodeId id)
{
    if (node.id() == id) {
        return &node;
    }
    for (const auto& child : node.children()) {
        if (Node* found = findById(*child, id)) {
            return found;
        }
    }

    return nullptr;
}

} // namespace

// Вложен в SceneSerializer, чтобы вызывать закрытый assignId.
struct SceneSerializer::Reader {
    const TypeRegistry& registry;

    std::unique_ptr<Node> readNode(const Json& json) const
    {
        if (!json.is_object() || !json.contains("type") || !json["type"].is_string()) {
            log::error("Scene: node without a type");

            return nullptr;
        }
        const std::string typeName = json["type"].get<std::string>();
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

        const auto id = json.contains("id") ? idFromJson(json["id"]) : std::nullopt;
        if (id && *id != 0) {
            assignId(*node, *id);
        }
        if (const auto it = json.find("properties"); it != json.end()) {
            propertiesFromJson(registry, *type, *node, *it);
        }
        if (const auto it = json.find("components"); it != json.end() && it->is_array()) {
            for (const Json& componentJson : *it) {
                readComponent(*node, componentJson);
            }
        }
        if (const auto it = json.find("children"); it != json.end() && it->is_array()) {
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
        if (!json.is_object() || !json.contains("type") || !json["type"].is_string()) {
            log::warn("Scene: component without a type on node '{}', skipped", node.name());

            return;
        }
        const std::string typeName = json["type"].get<std::string>();
        const TypeInfo* type = registry.find(typeName);
        if (!type || type->kind() != TypeKind::Component) {
            log::warn("Scene: unknown component type '{}' on node '{}', skipped", typeName, node.name());

            return;
        }

        Component* component = nullptr;
        if (type->creatable()) {
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
        if (const auto it = json.find("properties"); it != json.end()) {
            propertiesFromJson(registry, *type, *component, *it);
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

std::string SceneSerializer::toJson(const Scene& scene) const
{
    Json json;
    json["version"] = kFormatVersion;
    json["clearColor"] = vectorToJson(scene.clearColor());
    json["activeCamera"] = idToJson(scene.activeCamera() ? scene.activeCamera()->id() : NodeId{0});
    json["root"] = scene.root() ? nodeToJson(m_registry, *scene.root()) : Json(nullptr);

    return json.dump(4);
}

bool SceneSerializer::fromJson(Scene& scene, std::string_view text) const
{
    Json json = Json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (json.is_discarded() || !json.is_object()) {
        log::error("Scene: file is not a valid JSON object");

        return false;
    }
    if (!migrate(json)) {
        return false;
    }

    // Дерево собираем отдельно и отдаём сцене, только если всё прочиталось.
    std::unique_ptr<Node> root;
    if (const auto it = json.find("root"); it != json.end() && !it->is_null()) {
        root = Reader{m_registry}.readNode(*it);
        if (!root) {
            return false;
        }
    }

    scene.setRoot(std::move(root));
    if (const auto it = json.find("clearColor"); it != json.end()) {
        if (const auto color = vectorFromJson<Vec4>(*it)) {
            scene.setClearColor(*color);
        }
    }
    const auto camera = json.contains("activeCamera") ? idFromJson(json["activeCamera"]) : std::nullopt;
    if (camera && scene.root()) {
        Node* node = findById(*scene.root(), *camera);
        scene.setActiveCamera(dynamic_cast<Camera3D*>(node));
    }

    return true;
}

bool SceneSerializer::save(const Scene& scene, const std::filesystem::path& file) const
{
    std::ofstream out(file);
    if (!out) {
        log::error("Scene: cannot write '{}'", file.string());

        return false;
    }
    out << toJson(scene) << '\n';

    return static_cast<bool>(out);
}

bool SceneSerializer::load(Scene& scene, const std::filesystem::path& file) const
{
    std::ifstream in(file);
    if (!in) {
        log::error("Scene: cannot open '{}'", file.string());

        return false;
    }
    std::stringstream text;
    text << in.rdbuf();

    return fromJson(scene, text.str());
}

} // namespace yqlora
