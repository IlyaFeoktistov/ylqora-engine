#include "yqlora/editor/Inspection.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/reflection/ComponentRules.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Node.hpp"

namespace yqlora::editor {

std::optional<Inspection> inspect(EditorSession& session)
{
    const TypeRegistry& registry = session.registry();
    Node* node = session.selectedNode();
    const TypeInfo* type = node ? registry.find(node->typeName()) : nullptr;
    if (!node || !type) {
        return std::nullopt;
    }

    Inspection result{.node = *node, .type = *type, .components = {}, .addable = attachableTypes(registry, *node)};
    for (const auto& component : node->components()) {
        result.components.push_back({
            .component = *component,
            .type = registry.find(component->typeName()),
            .removable = canDetach(registry, *node, *component),
            .requiredBy = dependentsOf(registry, *node, *component),
        });
    }

    return result;
}

} // namespace yqlora::editor
