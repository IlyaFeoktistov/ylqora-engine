#include "Theme.hpp"

#include "yqlora/core/Log.hpp"

#include <imgui.h>
// ImGuizmo.h использует типы ImGui и должен идти после imgui.h.
#include <ImGuizmo.h>
#include <nlohmann/json.hpp>

#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace yqlora::theme {

namespace {

using Json = nlohmann::json;

EditorColors g_colors;

// "#RRGGBB" или "#RRGGBBAA".
std::optional<Vec4> parseColor(const Json& json)
{
    if (!json.is_string()) {
        return std::nullopt;
    }
    const std::string text = json.get<std::string>();
    if (text.size() != 7 && text.size() != 9) {
        return std::nullopt;
    }
    if (text.front() != '#') {
        return std::nullopt;
    }
    float channels[4] = {0.0f, 0.0f, 0.0f, 255.0f};
    for (std::size_t i = 0; i < (text.size() - 1) / 2; ++i) {
        try {
            channels[i] = static_cast<float>(std::stoul(text.substr(1 + i * 2, 2), nullptr, 16));
        } catch (const std::exception&) {
            return std::nullopt;
        }
    }

    const Vec4 bytes(channels[0], channels[1], channels[2], channels[3]);

    return bytes / 255.0f;
}

void applyColors(const Json& json, ImGuiStyle& style)
{
    if (!json.is_object()) {
        return;
    }
    for (int i = 0; i < ImGuiCol_COUNT; ++i) {
        const auto it = json.find(ImGui::GetStyleColorName(i));
        if (it == json.end()) {
            continue;
        }
        if (const auto color = parseColor(*it)) {
            style.Colors[i] = ImVec4(color->r, color->g, color->b, color->a);
        } else {
            log::warn("Theme: color '{}' is not #RRGGBB or #RRGGBBAA", it.key());
        }
    }
}

void applyStyle(const Json& json, ImGuiStyle& style)
{
    if (!json.is_object()) {
        return;
    }

    struct FloatField {
        std::string_view name;
        float ImGuiStyle::* field;
    };
    struct VectorField {
        std::string_view name;
        ImVec2 ImGuiStyle::* field;
    };
    static constexpr FloatField kFloats[] = {
        {"indentSpacing", &ImGuiStyle::IndentSpacing},
        {"scrollbarSize", &ImGuiStyle::ScrollbarSize},
        {"windowRounding", &ImGuiStyle::WindowRounding},
        {"childRounding", &ImGuiStyle::ChildRounding},
        {"frameRounding", &ImGuiStyle::FrameRounding},
        {"popupRounding", &ImGuiStyle::PopupRounding},
        {"scrollbarRounding", &ImGuiStyle::ScrollbarRounding},
        {"grabRounding", &ImGuiStyle::GrabRounding},
        {"tabRounding", &ImGuiStyle::TabRounding},
        {"windowBorderSize", &ImGuiStyle::WindowBorderSize},
        {"frameBorderSize", &ImGuiStyle::FrameBorderSize},
        {"popupBorderSize", &ImGuiStyle::PopupBorderSize},
    };
    static constexpr VectorField kVectors[] = {
        {"windowPadding", &ImGuiStyle::WindowPadding},
        {"framePadding", &ImGuiStyle::FramePadding},
        {"itemSpacing", &ImGuiStyle::ItemSpacing},
        {"itemInnerSpacing", &ImGuiStyle::ItemInnerSpacing},
    };

    for (const auto& [name, field] : kFloats) {
        if (const auto it = json.find(name); it != json.end() && it->is_number()) {
            style.*field = it->get<float>();
        }
    }
    for (const auto& [name, field] : kVectors) {
        const auto it = json.find(name);
        if (it != json.end() && it->is_array() && it->size() == 2 && (*it)[0].is_number() && (*it)[1].is_number()) {
            style.*field = ImVec2((*it)[0].get<float>(), (*it)[1].get<float>());
        }
    }
}

// Шрифт из описания {"file": ..., "size": ...}. merge - подмешать к предыдущему
// шрифту: так иконки встают прямо в текст.
void addFont(const Json& json, const std::filesystem::path& assetsDir, bool merge)
{
    if (!json.is_object() || !json.contains("file") || !json["file"].is_string()) {
        return;
    }
    const std::string file = json["file"].get<std::string>();
    const float size = json.contains("size") && json["size"].is_number() ? json["size"].get<float>() : 17.0f;
    const auto path = assetsDir / std::filesystem::path(std::u8string(file.begin(), file.end()));

    ImFontConfig config;
    config.MergeMode = merge;
    if (const auto it = json.find("offsetY"); it != json.end() && it->is_number()) {
        config.GlyphOffset.y = it->get<float>();
    }
    // Глифы подгружаются по мере надобности, кириллица в том числе.
    if (!ImGui::GetIO().Fonts->AddFontFromFileTTF(path.string().c_str(), size, &config)) {
        log::warn("Theme: font '{}' not found", path.string());
    }
}

void applyEditorColors(const Json& json)
{
    if (!json.is_object()) {
        return;
    }
    const auto read = [&](const char* name, Vec4& out) {
        if (const auto it = json.find(name); it != json.end()) {
            if (const auto color = parseColor(*it)) {
                out = *color;
            }
        }
    };
    read("errorText", g_colors.errorText);
    read("selectionOutline", g_colors.selectionOutline);
    read("cameraGizmo", g_colors.cameraGizmo);
    read("playMode", g_colors.playMode);
    read("axisX", g_colors.axisX);
    read("axisY", g_colors.axisY);
    read("axisZ", g_colors.axisZ);
    read("axisW", g_colors.axisW);
}

// Манипуляторы окрашены в те же цвета осей, что и поля X, Y, Z в инспекторе.
void applyManipulatorColors()
{
    ImGuizmo::Style& style = ImGuizmo::GetStyle();
    const auto set = [&](ImGuizmo::COLOR slot, const Vec4& color) {
        style.Colors[slot] = theme::toImVec4(color);
    };
    set(ImGuizmo::DIRECTION_X, g_colors.axisX);
    set(ImGuizmo::DIRECTION_Y, g_colors.axisY);
    set(ImGuizmo::DIRECTION_Z, g_colors.axisZ);
    set(ImGuizmo::PLANE_X, g_colors.axisX * Vec4(1.0f, 1.0f, 1.0f, 0.4f));
    set(ImGuizmo::PLANE_Y, g_colors.axisY * Vec4(1.0f, 1.0f, 1.0f, 0.4f));
    set(ImGuizmo::PLANE_Z, g_colors.axisZ * Vec4(1.0f, 1.0f, 1.0f, 0.4f));
    set(ImGuizmo::SELECTION, g_colors.selectionOutline);
}

} // namespace

bool load(const std::filesystem::path& file, const std::filesystem::path& assetsDir)
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGui::StyleColorsDark(&style);

    std::ifstream in(file);
    const Json json = in ? Json::parse(in, nullptr, /*allow_exceptions=*/false) : Json();
    if (!json.is_object()) {
        log::warn("Theme '{}' is not found or broken, using the default look", file.string());

        return false;
    }

    addFont(json.value("font", Json()), assetsDir, false);
    addFont(json.value("iconFont", Json()), assetsDir, true);
    applyStyle(json.value("style", Json()), style);
    applyColors(json.value("colors", Json()), style);
    applyEditorColors(json.value("editor", Json()));
    applyManipulatorColors();

    return true;
}

const EditorColors& colors()
{
    return g_colors;
}

} // namespace yqlora::theme
