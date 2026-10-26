#include "PropertyEditor.hpp"

#include "yqlora/i18n/Translate.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/render/MeshData.hpp"

#include <imgui.h>
#include <imgui_stdlib.h>

#include <cctype>
#include <string_view>

namespace yqlora {

namespace {

bool drawMeshCombo(std::string& value)
{
    bool changed = false;
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

// Поле для значения value. Подписи у самого поля нет: она в соседней колонке.
bool drawValue(const Property& property, PropertyValue& value)
{
    ImGui::SetNextItemWidth(-FLT_MIN);
    switch (property.type()) {
    case PropertyType::Bool:
        return ImGui::Checkbox("##value", &std::get<bool>(value));
    case PropertyType::Int:
        return ImGui::DragInt("##value", &std::get<int>(value));
    case PropertyType::Float:
        return ImGui::DragFloat("##value", &std::get<float>(value), 0.1f);
    case PropertyType::String:
        if (property.hint() == PropertyHint::Mesh) {
            return drawMeshCombo(std::get<std::string>(value));
        }

        return ImGui::InputText("##value", &std::get<std::string>(value));
    case PropertyType::Vector2:
        return ImGui::DragFloat2("##value", &std::get<Vec2>(value).x, 0.05f);
    case PropertyType::Vector3:
        return ImGui::DragFloat3("##value", &std::get<Vec3>(value).x, 0.05f);
    case PropertyType::Vector4:
        if (property.hint() == PropertyHint::Color) {
            return ImGui::ColorEdit4("##value", &std::get<Vec4>(value).x);
        }

        return ImGui::DragFloat4("##value", &std::get<Vec4>(value).x, 0.05f);
    }

    return false;
}

template <typename Object>
bool drawPropertyRow(const Property& property, Object& object)
{
    ImGui::PushID(property.name().c_str());
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted(propertyLabel(property.name()).c_str());
    ImGui::TableNextColumn();

    PropertyValue value = property.get(object);
    const bool changed = drawValue(property, value);
    if (changed) {
        property.set(object, value);
    }
    ImGui::PopID();

    return changed;
}

} // namespace

bool drawProperty(const Property& property, Node& node)
{
    return drawPropertyRow(property, node);
}

bool drawProperty(const Property& property, Component& component)
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
