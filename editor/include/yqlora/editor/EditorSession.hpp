#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/editor/EditHistory.hpp"
#include "yqlora/editor/Project.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/editor/TransformEdit.hpp"
#include "yqlora/reflection/Property.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

namespace yqlora {

class Component;
class Node;
class Node3D;
class Scene;
class SceneSerializer;
class TypeInfo;

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

    Scene& scene()
    {
        return m_scene;
    }

    Selection& selection()
    {
        return m_selection;
    }

    const Selection& selection() const
    {
        return m_selection;
    }

    // Открывает проект и его главную сцену. Если сцена не читается, проект
    // всё равно открывается с пустой сценой, а метод возвращает false.
    bool open(const Project& project);
    void close();

    // Открытая сцена, путь от папки проекта.
    const std::filesystem::path& scenePath() const
    {
        return m_scenePath;
    }

    // Открывает другую сцену проекта. Если файл не читается, остаётся прежняя.
    bool openScene(const std::filesystem::path& path);

    // Сохраняет сцену в её файл.
    bool saveScene();

    // Делает сцену главной: она откроется вместе с проектом.
    bool setMainScene(const std::filesystem::path& path);

    // Файл или папку переименовали или удалили в панели файлов: если это
    // открытая сцена (или папка с ней), путь к ней обновляется.
    void fileRenamed(const std::filesystem::path& from, const std::filesystem::path& to);

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
        return !m_playing && m_history.canUndo();
    }

    bool canRedo() const
    {
        return !m_playing && m_history.canRedo();
    }

    bool undo();
    bool redo();

    bool playing() const
    {
        return m_playing;
    }

    bool paused() const
    {
        return m_paused;
    }

    // Сцене пора обновляться: игра идёт и не на паузе.
    bool running() const
    {
        return m_playing && !m_paused;
    }

    // Запоминает сцену и запускает игру. Правки во время игры отбрасываются по Stop.
    void play();
    void setPaused(bool paused);
    // Останавливает игру и возвращает сцену и выделение, какими они были до Play.
    void stop();

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

    // nullptr, если тип не компонент или его нельзя создать.
    Component* addComponent(Node& node, const TypeInfo& type);
    bool removeComponent(Node& node, Component& component);

    bool setProperty(Node& node, const Property& property, const PropertyValue& value);
    bool setProperty(Component& component, const Property& property, const PropertyValue& value);

    bool setTransform(Node3D& node, const TransformValues& values);

private:
    Scene& m_scene;
    const SceneSerializer& m_serializer;
    std::optional<Project> m_project;
    std::filesystem::path m_scenePath;
    Selection m_selection;
    EditHistory m_history;

    bool m_playing = false;
    bool m_paused = false;
    // Сцена до Play, в виде файла сцены: так восстанавливаются и ID узлов.
    std::string m_snapshot;
    NodeId m_selectedBeforePlay = 0;
};

} // namespace yqlora
