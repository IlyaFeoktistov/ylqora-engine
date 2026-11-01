#include "yqlora/editor/ResourceLabel.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/BuiltinMeshes.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"

namespace yqlora::editor {

std::string subresourceLabel(std::string_view key)
{
    // Ключ без вида: "Door", "Door#1", "#2" - номер отличает одинаковые имена.
    const std::size_t colon = key.find(':');

    return std::string(colon == std::string_view::npos ? key : key.substr(colon + 1));
}

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
        const std::string_view key = resources->key(value.id);
        if (key.empty()) {
            return toGenericUtf8(*path);
        }

        return toGenericUtf8(*path) + " / " + subresourceLabel(key);
    }

    return tr("editor.inspector.missing");
}

} // namespace yqlora::editor
