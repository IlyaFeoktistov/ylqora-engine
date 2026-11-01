#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace yqlora {

class TypeInfo;

} // namespace yqlora

namespace yqlora::editor {

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
};

// Новая ветка (узел или целое дерево, например модель из файла) ребёнком
// parent (0 - ребёнком корня, а в пустой сцене - корнем). Имя её корня
// уникально среди соседей. Корень ветки выбирается.
class AddBranchCommand final : public Command {
public:
    // Ветка готова заранее, поэтому ID её корня известен до apply.
    AddBranchCommand(std::unique_ptr<Node> branch, NodeId parent);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

    // 0, если ветки нет.
    NodeId created() const
    {
        return m_created;
    }

private:
    NodeId m_parent;
    NodeId m_created = 0;
    bool m_placed = false;
    Selection m_previousSelection;
    DetachedBranch m_branch;
};

// Удаление узла со всеми детьми. Выделение внутри ветки при отмене
// возвращается.
class DeleteNodeCommand final : public Command {
public:
    explicit DeleteNodeCommand(NodeId node);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

private:
    NodeId m_node;
    Selection m_previousSelection;
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

// Несколько команд одним шагом истории, например удаление нескольких узлов.
// Делаются по порядку, отменяются в обратном. Если одна не удалась,
// сделанные до неё отменяются и вся команда не удаётся.
class CompositeCommand final : public Command {
public:
    explicit CompositeCommand(std::vector<std::unique_ptr<Command>> commands);

    bool apply(EditorSession& session) override;
    void revert(EditorSession& session) override;

private:
    std::vector<std::unique_ptr<Command>> m_commands;
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

} // namespace yqlora::editor
