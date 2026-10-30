#include "yqlora/editor/ResourceLabel.hpp"

#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/i18n/Translate.hpp"
#include "yqlora/render/BuiltinMeshes.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"

namespace yqlora::editor {

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

} // namespace yqlora::editor
