#include "widgets/PropertyEditor.hpp"

#include "style/Theme.hpp"
#include "widgets/Checkbox.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/render/MeshData.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"
#include "yqlora/scene/Layers.hpp"
#include "yqlora/scene/Scene.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <algorithm>
#include <bit>
#include <cctype>
#include <string_view>
#include <utility>
#include <vector>

namespace yqlora::editor {

namespace {

constexpr float kFastFactor = 10.0f; // с Shift
constexpr float kSlowFactor = 0.1f;  // с Ctrl

// Сколько меняется значение за точку движения мыши.
constexpr float kAngleDragSpeed = 0.5f;
constexpr float kFloatDragSpeed = 0.05f;
constexpr float kVectorDragSpeed = 0.02f;

float dragSpeed(const Property& property)
{
    switch (property.hint()) {
    case PropertyHint::Angle: return kAngleDragSpeed;
    default: return property.type() == PropertyType::Float ? kFloatDragSpeed : kVectorDragSpeed;
    }
}

// Подпись, которую можно тянуть мышью, как в Unity. Меняет value и
// отмечает в result, что значение тянут.
void dragLabel(const char* id, const char* text, const ImVec4& color, float& value, float speed, PropertyResult& result)
{
    const ImVec2 textSize = ImGui::CalcTextSize(text);
    const ImVec2 padding = ImGui::GetStyle().FramePadding;
    const ImVec2 size(textSize.x + padding.x * 2.0f, ImGui::GetFrameHeight());
    const ImVec2 position = ImGui::GetCursorScreenPos();

    ImGui::InvisibleButton(id, size);
    const bool active = ImGui::IsItemActive();
    if (active || ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
    }
    const ImVec2 textPosition(position.x + padding.x, position.y + padding.y);
    ImGui::GetWindowDrawList()->AddText(textPosition, ImGui::GetColorU32(color), text);

    if (!active) {
        return;
    }
    result.dragging = true;
    const ImGuiIO& io = ImGui::GetIO();
    const float factor = io.KeyShift ? kFastFactor : io.KeyCtrl ? kSlowFactor : 1.0f;
    const float delta = io.MouseDelta.x * speed * factor;
    if (delta != 0.0f) {
        value += delta;
        result.changed = true;
    }
}

// Поле числа без подписи заданной ширины. format влияет только на показ:
// введённое значение сохраняется с полной точностью.
void floatField(const char* id, float& value, float width, const char* format, PropertyResult& result)
{
    ImGui::SetNextItemWidth(width);
    if (ImGui::InputFloat(id, &value, 0.0f, 0.0f, format)) {
        result.changed = true;
    }
}

// Компоненты вектора в одну строку: у каждой своя цветная подпись.
void vectorFields(float* values, int count, float speed, const EditorColors& colors, PropertyResult& result)
{
    static constexpr const char* kNames[] = {"X", "Y", "Z", "W"};
    const ImVec4 axisColors[] =
        {toImVec4(colors.axisX), toImVec4(colors.axisY), toImVec4(colors.axisZ), toImVec4(colors.axisW)};

    const ImGuiStyle& style = ImGui::GetStyle();
    const float labelWidth = ImGui::CalcTextSize("X").x + style.FramePadding.x * 2.0f;
    const float total = ImGui::GetContentRegionAvail().x;
    const float spacing = style.ItemInnerSpacing.x;
    const float fieldWidth = (total - static_cast<float>(count) * labelWidth - static_cast<float>(count - 1) * spacing)
                             / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        ImGui::PushID(i);
        if (i > 0) {
            ImGui::SameLine(0.0f, spacing);
        }
        dragLabel("##drag", kNames[i], axisColors[i], values[i], speed, result);
        ImGui::SameLine(0.0f, 0.0f);
        floatField("##value", values[i], fieldWidth, "%.2f", result);
        ImGui::PopID();
    }
}

// Ресурс по имени: у встроенных - имя меша, у ресурсов проекта - путь к файлу.
std::string resourceLabel(ResourceRef value, const ResourceDatabase* resources)
{
    if (value.id == 0) {
        return tr("editor.inspector.none");
    }
    for (const BuiltinMesh& mesh : builtinMeshes()) {
        if (mesh.id == value.id) {
            return std::string(mesh.name);
        }
    }
    if (const auto path = resources ? resources->path(value.id) : std::nullopt) {
        return toGenericUtf8(*path);
    }

    return tr("editor.inspector.missing");
}

// Пока выбираются только встроенные меши: загрузка мешей из файлов будет позже.
bool meshCombo(ResourceRef& value, const ResourceDatabase* resources)
{
    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##value", resourceLabel(value, resources).c_str())) {
        for (const BuiltinMesh& mesh : builtinMeshes()) {
            const std::string name(mesh.name);
            if (ImGui::Selectable(name.c_str(), mesh.id == value.id)) {
                value.id = mesh.id;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}

// Подпись слоя. Пока номер: имена слоёв придут с настройками проекта.
std::string layerLabel(int layer)
{
    return std::to_string(layer);
}

bool layerCombo(int& layer)
{
    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##value", layerLabel(layer).c_str())) {
        for (int i = 0; i < kLayerCount; ++i) {
            if (ImGui::Selectable(layerLabel(i).c_str(), i == layer)) {
                layer = i;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}

std::string maskPreview(LayerMask mask)
{
    if (mask == kAllLayers) {
        return tr("editor.layers.everything");
    }
    if (mask == 0) {
        return tr("editor.layers.nothing");
    }
    std::string result;
    for (int i = 0; i < kLayerCount; ++i) {
        if (inMask(mask, i)) {
            result += (result.empty() ? "" : ", ") + layerLabel(i);
        }
    }

    return result;
}

// Маска хранится в int с теми же битами, см. PropertyStorage<std::uint32_t>.
bool layerMaskCombo(int& stored, float checkboxPadding)
{
    auto mask = std::bit_cast<LayerMask>(stored);
    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##value", maskPreview(mask).c_str())) {
        if (ImGui::Selectable(tr("editor.layers.everything").c_str(), mask == kAllLayers)) {
            mask = kAllLayers;
            changed = true;
        }
        if (ImGui::Selectable(tr("editor.layers.nothing").c_str(), mask == 0)) {
            mask = 0;
            changed = true;
        }
        ImGui::Separator();
        for (int i = 0; i < kLayerCount; ++i) {
            bool on = inMask(mask, i);
            ImGui::PushID(i);
            if (checkbox(layerLabel(i).c_str(), &on, checkboxPadding)) {
                mask ^= LayerMask{1} << i;
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    stored = std::bit_cast<int>(mask);

    return changed;
}

bool enumCombo(const Property& property, EnumValue& value)
{
    const auto& names = property.enumNames();
    const auto current = static_cast<std::size_t>(value.value);
    const std::string preview = value.value >= 0 && current < names.size() ? names[current]
                                                                           : std::to_string(value.value);

    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##value", preview.c_str())) {
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (ImGui::Selectable(names[i].c_str(), i == current)) {
                value.value = static_cast<int>(i);
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}

void collectNodes(const Node& node, std::vector<const Node*>& out)
{
    out.push_back(&node);
    for (const auto& child : node.children()) {
        collectNodes(*child, out);
    }
}

// Узел сцены по имени. Удалённый узел показывается как "нет".
bool nodeCombo(const Scene& scene, NodeRef& value)
{
    std::vector<const Node*> nodes;
    if (const Node* root = scene.root()) {
        collectNodes(*root, nodes);
    }
    const auto current = std::ranges::find(nodes, value.id, &Node::id);
    const std::string none = tr("editor.inspector.none");
    const char* preview = current != nodes.end() ? (*current)->name().c_str() : none.c_str();

    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##value", preview)) {
        if (ImGui::Selectable(none.c_str(), value.id == 0)) {
            value.id = 0;
            changed = true;
        }
        for (const Node* node : nodes) {
            ImGui::PushID(node);
            if (ImGui::Selectable(node->name().c_str(), node->id() == value.id)) {
                value.id = node->id();
                changed = true;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }

    return changed;
}

// Подпись строки. У одиночного числа её можно тянуть, у остальных - просто текст.
void rowLabel(const Property& property, PropertyValue& value, PropertyResult& result)
{
    const std::string label = propertyLabel(property.name());
    if (property.type() == PropertyType::Float) {
        dragLabel(
            "##label",
            label.c_str(),
            ImGui::GetStyleColorVec4(ImGuiCol_Text),
            std::get<float>(value),
            dragSpeed(property),
            result
        );

        return;
    }
    // С тем же отступом, что у подписей, которые можно тянуть.
    ImGui::AlignTextToFramePadding();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().FramePadding.x);
    ImGui::TextUnformatted(label.c_str());
}

void valueField(
    const Property& property,
    PropertyValue& value,
    const PropertyEditorContext& context,
    PropertyResult& result
)
{
    const float speed = dragSpeed(property);
    switch (property.type()) {
    case PropertyType::Bool:
        result.changed = checkbox("##value", &std::get<bool>(value), context.metrics.checkboxPadding) || result.changed;
        break;
    case PropertyType::Int:
        if (property.hint() == PropertyHint::Layer) {
            result.changed = layerCombo(std::get<int>(value)) || result.changed;
        } else if (property.hint() == PropertyHint::Layers) {
            result.changed = layerMaskCombo(std::get<int>(value), context.metrics.checkboxPadding) || result.changed;
        } else {
            ImGui::SetNextItemWidth(-FLT_MIN);
            result.changed = ImGui::DragInt("##value", &std::get<int>(value)) || result.changed;
        }
        break;
    case PropertyType::Float: floatField("##value", std::get<float>(value), -FLT_MIN, "%.3f", result); break;
    case PropertyType::String:
        ImGui::SetNextItemWidth(-FLT_MIN);
        result.changed = ImGui::InputText("##value", &std::get<std::string>(value)) || result.changed;
        break;
    case PropertyType::Vector2:
        vectorFields(&std::get<Vec2>(value).x, Vec2::length(), speed, context.colors, result);
        break;
    case PropertyType::Vector3:
        vectorFields(&std::get<Vec3>(value).x, Vec3::length(), speed, context.colors, result);
        break;
    case PropertyType::Vector4:
        if (property.hint() == PropertyHint::Color) {
            ImGui::SetNextItemWidth(-FLT_MIN);
            result.changed = ImGui::ColorEdit4("##value", &std::get<Vec4>(value).x) || result.changed;
        } else {
            vectorFields(&std::get<Vec4>(value).x, Vec4::length(), speed, context.colors, result);
        }
        break;
    case PropertyType::Enum: result.changed = enumCombo(property, std::get<EnumValue>(value)) || result.changed; break;
    case PropertyType::Node:
        result.changed = nodeCombo(context.scene, std::get<NodeRef>(value)) || result.changed;
        break;
    case PropertyType::Resource:
        if (property.hint() == PropertyHint::Mesh) {
            result.changed = meshCombo(std::get<ResourceRef>(value), context.resources) || result.changed;
        } else {
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted(resourceLabel(std::get<ResourceRef>(value), context.resources).c_str());
        }
        break;
    }
}

template <typename Object>
PropertyResult drawPropertyRow(const Property& property, Object& object, const PropertyEditorContext& context)
{
    PropertyResult result;
    PropertyValue value = property.get(object);

    ImGui::PushID(property.name().c_str());
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    rowLabel(property, value, result);
    ImGui::TableNextColumn();
    valueField(property, value, context, result);
    ImGui::PopID();

    result.value = std::move(value);

    return result;
}

} // namespace

PropertyResult drawProperty(const Property& property, Node& node, const PropertyEditorContext& context)
{
    return drawPropertyRow(property, node, context);
}

PropertyResult drawProperty(const Property& property, Component& component, const PropertyEditorContext& context)
{
    return drawPropertyRow(property, component, context);
}

std::string propertyLabel(const std::string& name)
{
    const std::string key = "property." + name;
    std::string label = tr(key);
    if (label == key) {
        label = name;
        if (!label.empty()) {
            label[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(label[0])));
        }
    }

    return label;
}

} // namespace yqlora::editor
