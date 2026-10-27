#include "yqlora/scene/SceneSerializer.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <optional>
#include <sstream>

namespace yqlora {

namespace {

using Json = nlohmann::json;

// Меняется, когда старые файлы перестают читаться новым кодом.
constexpr int kFormatVersion = 1;

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

Json valueToJson(const PropertyValue& value)
{
    return std::visit(
        [](const auto& v) -> Json {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, Vec2> || std::is_same_v<T, Vec3> || std::is_same_v<T, Vec4>) {
                return vectorToJson(v);
            } else {
                return Json(v);
            }
        },
        value
    );
}

std::optional<PropertyValue> valueFromJson(const Json& json, PropertyType type)
{
    switch (type) {
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
    }

    return std::nullopt;
}

template <typename Object>
Json propertiesToJson(const TypeRegistry& registry, const TypeInfo& type, const Object& object)
{
    Json result = Json::object();
    for (const Property* property : registry.properties(type)) {
        result[property->name()] = valueToJson(property->get(object));
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
        const auto value = valueFromJson(*it, property->type());
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
    result["id"] = node.id();
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

        if (const auto it = json.find("id"); it != json.end() && it->is_number_unsigned() && it->get<NodeId>() != 0) {
            assignId(*node, it->get<NodeId>());
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
    json["activeCamera"] = scene.activeCamera() ? scene.activeCamera()->id() : NodeId{0};
    json["root"] = scene.root() ? nodeToJson(m_registry, *scene.root()) : Json(nullptr);

    return json.dump(4);
}

bool SceneSerializer::fromJson(Scene& scene, std::string_view text) const
{
    const Json json = Json::parse(text, nullptr, /*allow_exceptions=*/false);
    if (json.is_discarded() || !json.is_object()) {
        log::error("Scene: file is not a valid JSON object");

        return false;
    }
    const auto version = json.find("version");
    if (version == json.end() || !version->is_number_integer() || version->get<int>() != kFormatVersion) {
        log::error("Scene: unsupported format version");

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
    if (const auto it = json.find("activeCamera"); it != json.end() && it->is_number_unsigned() && scene.root()) {
        Node* node = findById(*scene.root(), it->get<NodeId>());
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
