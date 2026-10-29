#include "yqlora/editor/EditorSettings.hpp"

#include "yqlora/core/JsonFile.hpp"

#include <nlohmann/json.hpp>

namespace yqlora::editor {

namespace {

constexpr const char* kCameraPreviewKey = "cameraPreview";

} // namespace

EditorSettings::EditorSettings(std::filesystem::path file)
    : m_file(std::move(file))
{
}

void EditorSettings::load()
{
    const auto json = readJsonFile(m_file);
    if (!json || !json->is_object()) {
        return;
    }
    if (const auto it = json->find(kCameraPreviewKey); it != json->end() && it->is_boolean()) {
        cameraPreview = it->get<bool>();
    }
}

bool EditorSettings::save() const
{
    return writeJsonFile(m_file, {{kCameraPreviewKey, cameraPreview}});
}

} // namespace yqlora::editor
