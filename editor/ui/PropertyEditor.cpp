#include "PropertyEditor.hpp"

#include "Theme.hpp"

#include "yqlora/i18n/Translate.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/render/MeshData.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <cctype>
#include <string_view>

namespace yqlora {

namespace {

constexpr float kFastFactor = 10.0f; // с Shift
constexpr float kSlowFactor = 0.1f;  // с Ctrl

// Сколько меняется значение за точку движения мыши.
float dragSpeed(const Property& property)
{
    switch (property.hint()) {
    case PropertyHint::Angle:
        return 0.5f;
    default:
        return property.type() == PropertyType::Float ? 0.05f : 0.02f;
    }
}

ImVec4 toImVec4(const Vec4& color)
{
    return {color.r, color.g, color.b, color.a};
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
    ImGui::GetWindowDrawList()->AddText(ImVec2(position.x + padding.x, position.y + padding.y),
                                        ImGui::GetColorU32(color), text);

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
void vectorFields(float* values, int count, float speed, PropertyResult& result)
{
    static constexpr const char* kNames[] = {"X", "Y", "Z", "W"};
    const EditorColors& colors = theme::colors();
    const ImVec4 axisColors[] = {toImVec4(colors.axisX), toImVec4(colors.axisY), toImVec4(colors.axisZ),
                                 toImVec4(colors.axisW)};

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

bool meshCombo(std::string& value)
{
    bool changed = false;
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::BeginCombo("##value", value.c_str())) {
        for (const std::string_view name : builtinMeshNames()) {
            const std::string item(name);
            if (ImGui::Selectable(item.c_str(), item == value)) {
                value = item;
                changed = true;
            }
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
        dragLabel("##label", label.c_str(), ImGui::GetStyleColorVec4(ImGuiCol_Text), std::get<float>(value),
                  dragSpeed(property), result);

        return;
    }
    // С тем же отступом, что у подписей, которые можно тянуть.
    ImGui::AlignTextToFramePadding();
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetStyle().FramePadding.x);
    ImGui::TextUnformatted(label.c_str());
}

void valueField(const Property& property, PropertyValue& value, PropertyResult& result)
{
    const float speed = dragSpeed(property);
    switch (property.type()) {
    case PropertyType::Bool:
        result.changed = ImGui::Checkbox("##value", &std::get<bool>(value)) || result.changed;
        break;
    case PropertyType::Int:
        ImGui::SetNextItemWidth(-FLT_MIN);
        result.changed = ImGui::DragInt("##value", &std::get<int>(value)) || result.changed;
        break;
    case PropertyType::Float:
        floatField("##value", std::get<float>(value), -FLT_MIN, "%.3f", result);
        break;
    case PropertyType::String:
        if (property.hint() == PropertyHint::Mesh) {
            result.changed = meshCombo(std::get<std::string>(value)) || result.changed;
        } else {
            ImGui::SetNextItemWidth(-FLT_MIN);
            result.changed = ImGui::InputText("##value", &std::get<std::string>(value)) || result.changed;
        }
        break;
    case PropertyType::Vector2:
        vectorFields(&std::get<Vec2>(value).x, 2, speed, result);
        break;
    case PropertyType::Vector3:
        vectorFields(&std::get<Vec3>(value).x, 3, speed, result);
        break;
    case PropertyType::Vector4:
        if (property.hint() == PropertyHint::Color) {
            ImGui::SetNextItemWidth(-FLT_MIN);
            result.changed = ImGui::ColorEdit4("##value", &std::get<Vec4>(value).x) || result.changed;
        } else {
            vectorFields(&std::get<Vec4>(value).x, 4, speed, result);
        }
        break;
    }
}

template <typename Object>
PropertyResult drawPropertyRow(const Property& property, Object& object)
{
    PropertyResult result;
    PropertyValue value = property.get(object);

    ImGui::PushID(property.name().c_str());
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    rowLabel(property, value, result);
    ImGui::TableNextColumn();
    valueField(property, value, result);
    ImGui::PopID();

    if (result.changed) {
        property.set(object, value);
    }

    return result;
}

} // namespace

PropertyResult drawProperty(const Property& property, Node& node)
{
    return drawPropertyRow(property, node);
}

PropertyResult drawProperty(const Property& property, Component& component)
{
    return drawPropertyRow(property, component);
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

} // namespace yqlora
