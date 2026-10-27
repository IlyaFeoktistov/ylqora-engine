#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/editor/TransformEdit.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Node.hpp"

#include <cstddef>
#include <memory>
#include <optional>

namespace yqlora {

class Camera3D;
class TypeInfo;

// Узел или компонент узла, чьё свойство меняют. Компонент задан местом
// в списке: после Stop компоненты пересоздаются в том же порядке.
struct PropertyTarget {
    NodeId node = 0;
    // nullopt - свойство самого узла.
    std::optional<std::size_t> component;

    bool operator==(const PropertyTarget&) const = default;
};

// Ветка дерева, вынутая из сцены, и место, куда её вернуть.
struct DetachedBranch {
    std::unique_ptr<Node> node;
    // 0 - ветка была корнем сцены.
    NodeId parent = 0;
    std::size_t index = 0;
    // Активная камера, если она была внутри ветки.
    Camera3D* activeCamera = nullptr;
};

// Новый узел типа type ребёнком parent (0 - ребёнком корня, а в пустой
// сцене - корнем). Имя уникально среди соседей. Новый узел выбирается.
class CreateNodeCommand final : public Command {
public:
    // Узел создаётся сразу, чтобы его ID был известен до apply.
    CreateNodeCommand(const TypeInfo& type, NodeId parent);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

    // 0, если тип не узел или его нельзя создать.
    NodeId created() const
    {
        return m_created;
    }

private:
    const TypeInfo& m_type;
    NodeId m_parent;
    NodeId m_created = 0;
    bool m_placed = false;
    NodeId m_previousSelection = 0;
    DetachedBranch m_branch;
};

// Удаление узла со всеми детьми. Активная камера и выделение внутри
// ветки при отмене возвращаются.
class DeleteNodeCommand final : public Command {
public:
    explicit DeleteNodeCommand(NodeId node);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

private:
    NodeId m_node;
    NodeId m_previousSelection = 0;
    DetachedBranch m_branch;
};

class AddComponentCommand final : public Command {
public:
    AddComponentCommand(NodeId node, const TypeInfo& type);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

private:
    NodeId m_node;
    const TypeInfo& m_type;
    std::optional<std::size_t> m_index;
    std::unique_ptr<Component> m_detached;
};

// Встроенный компонент (Transform) снять нельзя.
class RemoveComponentCommand final : public Command {
public:
    RemoveComponentCommand(NodeId node, std::size_t index);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

private:
    NodeId m_node;
    std::size_t m_index;
    std::unique_ptr<Component> m_detached;
};

// Правки одного свойства сливаются, пока не вызван finishEdit.
class SetPropertyCommand final : public Command {
public:
    SetPropertyCommand(PropertyTarget target, const Property& property, PropertyValue value);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;
    bool merge(const Command& next) override;

private:
    PropertyTarget m_target;
    const Property& m_property;
    PropertyValue m_value;
    std::optional<PropertyValue> m_previous;
};

// Положение, поворот и масштаб разом, например от манипулятора.
class SetTransformCommand final : public Command {
public:
    SetTransformCommand(NodeId node, const TransformValues& values);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;
    bool merge(const Command& next) override;

private:
    NodeId m_node;
    TransformValues m_values;
    std::optional<TransformValues> m_previous;
};

} // namespace yqlora
