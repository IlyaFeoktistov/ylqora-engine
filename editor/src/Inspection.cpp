#include "yqlora/editor/Inspection.hpp"

#include "yqlora/editor/EditorSession.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Node.hpp"

namespace yqlora::editor {

std::optional<Inspection> inspect(EditorSession& session)
{
    Node* node = session.selectedNode();
    const TypeInfo* type = node ? session.registry().find(node->typeName()) : nullptr;
    if (!node || !type) {
        return std::nullopt;
    }

    Inspection result{.node = *node, .type = *type, .components = {}, .addable = session.addableComponents(*node)};
    for (const auto& component : node->components()) {
        result.components.push_back({
            .component = *component,
            .type = session.registry().find(component->typeName()),
            .removable = session.canRemoveComponent(*node, *component),
            .requiredBy = session.componentDependents(*node, *component),
        });
    }

    return result;
}

} // namespace yqlora::editor
