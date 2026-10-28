#pragma once

#include <optional>
#include <vector>

namespace yqlora {

class Component;
class Node;
class TypeInfo;

} // namespace yqlora

namespace yqlora::editor {

class EditorSession;

struct InspectedComponent {
    Component& component;
    // nullptr, если тип компонента не зарегистрирован.
    const TypeInfo* type = nullptr;
    // См. EditorSession::canRemoveComponent.
    bool removable = false;
    // Кому компонент нужен; пока они есть, его не снять.
    std::vector<const Component*> requiredBy;
};

// Что показывает инспектор для выбранного узла. Собирается без интерфейса,
// панель только рисует готовое и вызывает команды сессии.
struct Inspection {
    Node& node;
    const TypeInfo& type;
    std::vector<InspectedComponent> components;
    // См. EditorSession::addableComponents.
    std::vector<const TypeInfo*> addable;
};

// nullopt, если ничего не выбрано или тип узла не зарегистрирован.
std::optional<Inspection> inspect(EditorSession& session);

} // namespace yqlora::editor
