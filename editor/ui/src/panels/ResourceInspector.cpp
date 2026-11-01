#include "yqlora/editor/ui/panels/ResourceInspector.hpp"

#include "yqlora/editor/ui/core/EditorContext.hpp"
#include "yqlora/editor/ui/core/EditorHost.hpp"
#include "yqlora/editor/ui/style/Icons.hpp"
#include "yqlora/editor/ui/style/Theme.hpp"
#include "yqlora/editor/ui/widgets/ResourceDrag.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/import/ModelData.hpp"
#include "yqlora/import/TextureImport.hpp"
#include "yqlora/render/Renderer.hpp"

#include <imgui.h>

#include <algorithm>
#include <span>
#include <string>

namespace yqlora::editor {

namespace {

// Масштаб импорта: меньше - модель исчезла бы.
constexpr float kMinImportScale = 0.0001f;
constexpr float kImportScaleSpeed = 0.01f;
// Превью не выше этой доли ширины панели.
constexpr float kPreviewMaxAspect = 1.0f;
constexpr int kMeshColumns = 4;

template <typename E>
bool enumCombo(const char* label, std::span<const NamedValue<E>> names, E& value)
{
    const auto current = std::ranges::find(names, value, &NamedValue<E>::value);
    bool changed = false;
    if (ImGui::BeginCombo(label, std::string(current->name).c_str())) {
        for (const NamedValue<E>& named : names) {
            if (ImGui::Selectable(std::string(named.name).c_str(), named.value == value)) {
                value = named.value;
                changed = true;
            }
        }
        ImGui::EndCombo();
    }

    return changed;
}

} // namespace

void ResourceInspector::draw(EditorContext& context, const ResourceInspection& inspection)
{
    if (inspection.id != m_id) {
        m_id = inspection.id;
        m_model = inspection.modelImport;
        m_texture = inspection.textureImport;
    }
    const bool model = inspection.kind == ModelData::kResourceKind;
    const bool texture = inspection.kind == TextureData::kResourceKind;
    const char* icon = model ? icons::kModelFile : texture ? icons::kImageFile : icons::kFile;
    ImGui::Text("%s  %s", icon, toUtf8(inspection.path.filename()).c_str());
    ImGui::TextDisabled(
        "%s",
        tr(model     ? "editor.resource.model"
           : texture ? "editor.resource.texture"
                     : "editor.resource.file")
            .c_str()
    );
    ImGui::SetItemTooltip("%s", toGenericUtf8(inspection.path).c_str());

    if (inspection.status && inspection.status->state != ResourceState::Ready) {
        const char* key = inspection.status->state == ResourceState::Missing ? "editor.resource.missing"
                                                                             : "editor.resource.notLoaded";
        ImGui::PushTextWrapPos(0.0f);
        ImGui::TextColored(
            toImVec4(context.theme.colors().errorText),
            "%s: %s",
            tr(key).c_str(),
            inspection.status->message.c_str()
        );
        ImGui::PopTextWrapPos();
    }
    if (model) {
        drawModel(inspection);
    } else if (texture) {
        drawTexture(context, inspection);
    }
    if (model || texture) {
        drawImportSettings(context, inspection);
    }
}

void ResourceInspector::drawModel(const ResourceInspection& inspection)
{
    ImGui::Spacing();
    ImGui::Text("%s: %zu", tr("editor.resource.nodes").c_str(), inspection.nodes);
    ImGui::SeparatorText(tr("editor.resource.meshes").c_str());
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchProp;
    if (ImGui::BeginTable("meshes", kMeshColumns, flags)) {
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(tr("editor.resource.vertices").c_str(), ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn(tr("editor.resource.triangles").c_str(), ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableSetupColumn(tr("editor.resource.submeshes").c_str(), ImGuiTableColumnFlags_WidthFixed);
        ImGui::TableHeadersRow();
        for (const MeshSummary& mesh : inspection.meshes) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            const std::string label = std::string(icons::kMesh) + "  " + mesh.label;
            ImGui::Selectable(label.c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
            resourceDragSource(mesh.id, label);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", mesh.vertices);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", mesh.triangles);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", mesh.submeshes);
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("%s", tr("editor.resource.dragHint").c_str());
}

void ResourceInspector::drawTexture(EditorContext& context, const ResourceInspection& inspection)
{
    const LoadedData<TextureData>& texture = inspection.texture;
    if (!texture) {
        return;
    }
    ImGui::Spacing();
    ImGui::Text("%s: %d x %d", tr("editor.resource.size").c_str(), texture.data->width, texture.data->height);
    const UiTexture ui = context.host.renderer().uiTexture(inspection.id, texture.revision, *texture.data);
    const float width = ImGui::GetContentRegionAvail().x;
    const float aspect = static_cast<float>(texture.data->height) / static_cast<float>(texture.data->width);
    const float height = width * std::min(aspect, kPreviewMaxAspect);
    const float shownWidth = height / aspect;
    ImGui::Image(
        static_cast<ImTextureID>(ui.id),
        ImVec2(shownWidth, height),
        ImVec2(ui.uv0.x, ui.uv0.y),
        ImVec2(ui.uv1.x, ui.uv1.y)
    );
}

void ResourceInspector::drawImportSettings(EditorContext& context, const ResourceInspection& inspection)
{
    ImGui::SeparatorText(tr("editor.resource.import").c_str());
    if (m_model) {
        ImGui::DragFloat(
            tr("editor.resource.scale").c_str(),
            &m_model->scale,
            kImportScaleSpeed,
            kMinImportScale,
            FLT_MAX,
            "%.4g",
            ImGuiSliderFlags_AlwaysClamp
        );
    }
    if (m_texture) {
        enumCombo<ColorSpace>(tr("editor.resource.colorSpace").c_str(), kColorSpaceNames, m_texture->colorSpace);
        enumCombo<TextureFilter>(tr("editor.resource.filter").c_str(), kTextureFilterNames, m_texture->filter);
        enumCombo<TextureWrap>(tr("editor.resource.wrap").c_str(), kTextureWrapNames, m_texture->wrap);
        ImGui::Checkbox(tr("editor.resource.mipmaps").c_str(), &m_texture->mipmaps);
    }

    const bool changed = m_model != inspection.modelImport || m_texture != inspection.textureImport;
    ImGui::BeginDisabled(!changed);
    if (ImGui::Button(tr("editor.resource.apply").c_str())) {
        if (m_model) {
            applyImportSettings(context.session, inspection.id, *m_model);
        }
        if (m_texture) {
            applyImportSettings(context.session, inspection.id, *m_texture);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button(tr("editor.resource.revert").c_str())) {
        m_model = inspection.modelImport;
        m_texture = inspection.textureImport;
    }
    ImGui::EndDisabled();
}

} // namespace yqlora::editor
