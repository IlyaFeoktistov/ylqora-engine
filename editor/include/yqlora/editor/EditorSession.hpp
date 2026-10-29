#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/editor/PlayMode.hpp"
#include "yqlora/editor/SceneDocument.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <span>

namespace yqlora {

class Component;
class Node;
class Node3D;
class Scene;
class SceneSerializer;
class TypeInfo;
class TypeRegistry;

} // namespace yqlora

namespace yqlora::editor {

// Открытый в редакторе проект: его сцена, выделение и игра в редакторе.
// Без интерфейса, панели только вызывают эти методы. Сцена меняется
// только командами, поэтому любую правку можно отменить. Сама сессия
// согласует части между собой (выделение при Play и Stop, история при
// открытии сцены); файл сцены и историю ведёт SceneDocument, игру - PlayMode.
class EditorSession {
public:
    EditorSession(Scene& scene, const SceneSerializer& serializer);

    const std::optional<Project>& project() const
    {
        return m_project;
    }

    std::optional<Project>& project()
    {
        return m_project;
    }

    // Сцена, с которой сейчас работают: во время игры - её копия для игры,
    // иначе редактируемая.
    Scene& scene()
    {
        return m_play.playing() ? *m_play.scene() : m_document.scene();
    }

    const Scene& scene() const
    {
        return m_play.playing() ? *m_play.scene() : m_document.scene();
    }

    const SceneDocument& document() const
    {
        return m_document;
    }

    const PlayMode& playMode() const
    {
        return m_play;
    }

    Selection& selection()
    {
        return m_selection;
    }

    const Selection& selection() const
    {
        return m_selection;
    }

    // Типы, которые знает сессия: те же, что у её сериализатора.
    const TypeRegistry& registry() const;

    // Открывает проект и его главную сцену. Если сцена не читается, проект
    // всё равно открывается с пустой сценой, а метод возвращает false.
    bool open(Project project);
    void close();

    // Путь к открытой сцене от папки проекта. Пусто, если сцены нет.
    std::filesystem::path scenePath() const;

    // Открывает другую сцену проекта. Если файл не читается, остаётся прежняя.
    bool openScene(const std::filesystem::path& path);

    // Сохраняет сцену в её файл. Во время игры нельзя: в сцене состояние игры.
    bool saveScene();

    // Делает сцену главной: она откроется вместе с проектом.
    bool setMainScene(const std::filesystem::path& path);

    // Во время игры правка в историю не попадает: по Stop сцена всё равно вернётся как была.
    bool execute(std::unique_ptr<Command> command);

    // Правка закончилась (отпустили мышь, ушли из поля ввода): следующая
    // будет отдельным шагом истории.
    void finishEdit()
    {
        m_document.history().finishEdit();
    }

    // Во время игры истории нет.
    bool canUndo() const
    {
        return !playing() && m_document.history().canUndo();
    }

    bool canRedo() const
    {
        return !playing() && m_document.history().canRedo();
    }

    bool undo();
    bool redo();

    bool playing() const
    {
        return m_play.playing();
    }

    // Запускает игру в копии редактируемой сцены, см. PlayMode.
    void play();
    void setPaused(bool paused)
    {
        m_play.setPaused(paused);
    }

    // Выбрасывает копию для игры и возвращается к редактируемой сцене.
    // Выделение остаётся на узлах, которые есть и в ней.
    void stop();

    // Кадр игры: обновляет копию сцены, пока игра идёт и не на паузе.
    void update(float deltaSeconds)
    {
        m_play.update(deltaSeconds);
    }

    // nullptr, если такого узла в сцене нет.
    Node* findNode(NodeId id);
    Node* selectedNode();

    // Правки ниже - короткие пути к командам из SceneCommands.hpp.

    // Создаёт узел типа type ребёнком parent. В пустой сцене узел становится
    // корнем, а без parent - ребёнком корня. Имя уникально среди соседей.
    // Новый узел выбирается. nullptr, если тип не узел или его нельзя создать.
    Node* createNode(const TypeInfo& type, Node* parent);

    // Удаляет узел со всеми детьми. Активная камера и выделение внутри удалённой
    // ветки сбрасываются, чтобы на них не осталось висячих указателей.
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
    const SceneSerializer& m_serializer;
    std::optional<Project> m_project;
    SceneDocument m_document;
    PlayMode m_play;
    Selection m_selection;
    // Выделение до Play: к нему возвращаемся, если после игры выбранных узлов не осталось.
    Selection m_selectionBeforePlay;
};

} // namespace yqlora::editor
