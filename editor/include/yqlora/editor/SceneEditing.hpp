#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/NodeId.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <memory>
#include <span>

namespace yqlora {
class Component;
class Node;
class Node3D;
class TypeInfo;
} // namespace yqlora

namespace yqlora::editor {

class EditorSession;

// Правка текущей сцены сессии командами с отменой: узлы, компоненты,
// свойства, трансформы. Методы ниже execute - короткие пути к командам из
// SceneCommands.hpp.
class SceneEditing {
public:
    explicit SceneEditing(EditorSession& session)
        : m_session(session)
    {
    }

    // Во время игры правка в историю не попадает: по Stop сцена всё равно вернётся как была.
    bool execute(std::unique_ptr<Command> command);

    // Правка закончилась (отпустили мышь, ушли из поля ввода): следующая
    // будет отдельным шагом истории.
    void finishEdit();

    // Во время игры истории нет.
    bool canUndo() const;
    bool canRedo() const;
    bool undo();
    bool redo();

    // Создаёт узел типа type ребёнком parent. В пустой сцене узел становится
    // корнем, а без parent - ребёнком корня. Имя уникально среди соседей.
    // Новый узел выбирается. nullptr, если тип не узел или его нельзя создать.
    Node* createNode(const TypeInfo& type, Node* parent);

    // Удаляет узел со всеми детьми. Выделение внутри удалённой ветки
    // сбрасывается, чтобы на него не осталось висячих указателей.
    bool deleteNode(NodeId id);

    // Удаляет несколько узлов одним шагом истории. Узлы внутри других
    // удаляемых веток пропускаются: они уходят вместе с веткой.
    bool deleteNodes(std::span<const NodeId> ids);

    // Добавляет компонент вместе с недостающими обязательными одним шагом
    // истории. nullptr, если тип нельзя создать или он не подходит узлу
    // (см. canAttach и missingRequirements в ComponentRules.hpp).
    Component* addComponent(Node& node, const TypeInfo& type);

    // Снимает компонент, если его можно снять (canDetach в ComponentRules.hpp).
    bool removeComponent(Node& node, Component& component);

    bool setProperty(Node& node, const Property& property, const PropertyValue& value);
    bool setProperty(Component& component, const Property& property, const PropertyValue& value);

    bool setTransform(Node3D& node, const TransformValues& values);

private:
    EditorSession& m_session;
};

} // namespace yqlora::editor
