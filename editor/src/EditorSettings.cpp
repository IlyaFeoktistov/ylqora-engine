#include "yqlora/editor/EditorSettings.hpp"

#include "yqlora/core/JsonFile.hpp"
#include "yqlora/core/Utf8Path.hpp"

#include <nlohmann/json.hpp>

namespace yqlora::editor {

namespace {

constexpr const char* kCameraPreviewKey = "cameraPreview";
constexpr const char* kReparentKeepWorldKey = "reparentKeepWorld";
constexpr const char* kToolchainKey = "toolchain";
constexpr const char* kCmakeKey = "cmake";
constexpr const char* kMakeProgramKey = "makeProgram";
constexpr const char* kCCompilerKey = "cCompiler";
constexpr const char* kCxxCompilerKey = "cxxCompiler";

// Поля переопределений с их ключами: одно место и для чтения, и для записи.
template <typename Overrides, typename Visitor>
void forEachOverride(Overrides& overrides, Visitor&& visit)
{
    visit(kCmakeKey, overrides.cmake);
    visit(kMakeProgramKey, overrides.makeProgram);
    visit(kCCompilerKey, overrides.cCompiler);
    visit(kCxxCompilerKey, overrides.cxxCompiler);
}

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
    if (const auto it = json->find(kReparentKeepWorldKey); it != json->end() && it->is_boolean()) {
        reparentKeepWorld = it->get<bool>() ? KeepWorld::Yes : KeepWorld::No;
    }
    if (const auto it = json->find(kToolchainKey); it != json->end() && it->is_object()) {
        forEachOverride(toolchain, [&](const char* key, std::filesystem::path& path) {
            if (const auto value = it->find(key); value != it->end() && value->is_string()) {
                path = fromUtf8(value->get<std::string>());
            }
        });
    }
}

bool EditorSettings::save() const
{
    nlohmann::json overrides = nlohmann::json::object();
    forEachOverride(toolchain, [&](const char* key, const std::filesystem::path& path) {
        if (!path.empty()) {
            overrides[key] = toUtf8(path);
        }
    });

    nlohmann::json json{{kCameraPreviewKey, cameraPreview}, {kToolchainKey, overrides}};
    if (reparentKeepWorld) {
        json[kReparentKeepWorldKey] = *reparentKeepWorld == KeepWorld::Yes;
    }

    return writeJsonFile(m_file, json);
}

} // namespace yqlora::editor
