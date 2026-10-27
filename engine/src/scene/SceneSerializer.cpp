#include "yqlora/scene/SceneSerializer.hpp"

#include "core/JsonId.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Camera3D.hpp"
#include "yqlora/scene/Scene.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <optional>

namespace yqlora {

namespace {

using Json = nlohmann::json;

// Версия формата файла сцены. Когда формат меняется, версия растёт, а в
// kMigrations добавляется перевод с предыдущей версии: старые сцены открываются.
// Ключи старой версии миграция объявляет у себя, а не берёт ключи ниже: если
// текущий формат что-то переименует, перевод со старой версии не изменится.
constexpr int kFirstFormatVersion = 1;
constexpr int kFormatVersion = 1;

// Ключи текущего формата.
constexpr const char* kVersionKey = "version";
constexpr const char* kRootKey = "root";
constexpr const char* kClearColorKey = "clearColor";
constexpr const char* kActiveCameraKey = "activeCamera";
constexpr const char* kTypeKey = "type";
constexpr const char* kIdKey = "id";
constexpr const char* kPropertiesKey = "properties";
constexpr const char* kComponentsKey = "components";
constexpr const char* kChildrenKey = "children";

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

using Migration = void (*)(Json& scene);

// kMigrations[i] переводит версию i + 1 в i + 2. Пока формат один, переводов нет.
constexpr std::array<Migration, kFormatVersion - kFirstFormatVersion> kMigrations{};

// Приводит файл к текущей версии. false, если версия неизвестна.
bool migrate(Json& scene)
{
    const auto it = scene.find(kVersionKey);
    if (it == scene.end() || !it->is_number_integer()) {
        log::error("Scene: no format version");

        return false;
    }
    const int version = it->get<int>();
    if (version < kFirstFormatVersion || version > kFormatVersion) {
        log::error("Scene: format version {} is not supported, this engine reads up to {}", version, kFormatVersion);

        return false;
    }
    for (int from = version; from < kFormatVersion; ++from) {
        kMigrations[from - kFirstFormatVersion](scene);
    }
    scene[kVersionKey] = kFormatVersion;

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
// когда в коде игры добавляют или убирают поле, уже сохранённые сцены открываются.
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
    json[kVersionKey] = kFormatVersion;
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
    if (!migrate(json)) {
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
        Node* node = findById(*scene.root(), *camera);
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
        log::error("Scene: cannot write '{}'", file.string());

        return false;
    }

    return true;
}

bool SceneSerializer::load(Scene& scene, const std::filesystem::path& file) const
{
    auto json = std::filesystem::exists(file) ? readJsonFile(file) : std::nullopt;
    if (!json) {
        log::error("Scene: cannot open '{}'", file.string());

        return false;
    }

    return Format{m_registry}.readScene(scene, std::move(*json));
}

} // namespace yqlora
