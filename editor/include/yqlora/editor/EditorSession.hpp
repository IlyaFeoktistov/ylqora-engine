#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/editor/EditHistory.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/editor/TransformEdit.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

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

// Открытый в редакторе проект: его сцена, выделение и история правок.
// Без интерфейса, панели только вызывают эти методы. Сцена меняется
// только командами, поэтому любую правку можно отменить.
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
        return m_playScene ? *m_playScene : m_editScene;
    }

    const Scene& scene() const
    {
        return m_playScene ? *m_playScene : m_editScene;
    }

    Selection& selection()
    {
        return m_selection;
    }

    // Типы, которые знает сессия: те же, что у её сериализатора.
    const TypeRegistry& registry() const;

    const Selection& selection() const
    {
        return m_selection;
    }

    // Открывает проект и его главную сцену. Если сцена не читается, проект
    // всё равно открывается с пустой сценой, а метод возвращает false.
    bool open(Project project);
    void close();

    // Открытая сцена как ресурс проекта: путь к ней меняется при
    // переименовании, а ID нет.
    ResourceId sceneId() const
    {
        return m_sceneId;
    }

    // Путь к открытой сцене от папки проекта. Пусто, если сцены нет.
    std::filesystem::path scenePath() const;

    // Открывает другую сцену проекта. Если файл не читается, остаётся прежняя.
    bool openScene(const std::filesystem::path& path);

    // Сохраняет сцену в её файл.
    bool saveScene();

    // Делает сцену главной: она откроется вместе с проектом.
    bool setMainScene(const std::filesystem::path& path);

    // Сцену меняли после открытия или последнего сохранения.
    bool modified() const
    {
        return !m_history.atSaved();
    }

    // Во время игры правка в историю не попадает: по Stop сцена всё равно вернётся как была.
    bool execute(std::unique_ptr<Command> command);

    // Правка закончилась (отпустили мышь, ушли из поля ввода): следующая
    // будет отдельным шагом истории.
    void finishEdit()
    {
        m_history.finishEdit();
    }

    // Во время игры истории нет.
    bool canUndo() const
    {
        return !playing() && m_history.canUndo();
    }

    bool canRedo() const
    {
        return !playing() && m_history.canRedo();
    }

    bool undo();
    bool redo();

    bool playing() const
    {
        return m_playScene != nullptr;
    }

    bool paused() const
    {
        return m_paused;
    }

    // Сцене пора обновляться: игра идёт и не на паузе.
    bool running() const
    {
        return playing() && !m_paused;
    }

    // Запускает игру в копии редактируемой сцены. Сама она во время игры не
    // меняется, правки идут в копию и пропадают вместе с ней по Stop.
    void play();
    void setPaused(bool paused);
    // Выбрасывает копию для игры и возвращается к редактируемой сцене.
    void stop();

    // Кадр игры: обновляет копию сцены, пока игра идёт и не на паузе.
    void update(float deltaSeconds);

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

    // Снимает компонент, если canRemoveComponent.
    bool removeComponent(Node& node, Component& component);

    // Единственное правило, можно ли снять компонент: не встроенный
    // и не нужен другим (см. componentDependents).
    bool canRemoveComponent(const Node& node, const Component& component) const;

    // Типы компонентов, которые addComponent сейчас добавит к узлу, в порядке регистрации.
    std::vector<const TypeInfo*> addableComponents(const Node& node) const;

    // Компоненты узла, которым нужен component; пока они есть, его не снять.
    std::vector<const Component*> componentDependents(const Node& node, const Component& component) const;

    bool setProperty(Node& node, const Property& property, const PropertyValue& value);
    bool setProperty(Component& component, const Property& property, const PropertyValue& value);

    bool setTransform(Node3D& node, const TransformValues& values);

private:
    Scene& m_editScene;
    std::unique_ptr<Scene> m_playScene;
    const SceneSerializer& m_serializer;
    std::optional<Project> m_project;
    ResourceId m_sceneId = 0;
    Selection m_selection;
    EditHistory m_history;

    bool m_paused = false;
    Selection m_selectionBeforePlay;
};

} // namespace yqlora::editor
