#pragma once

#include "yqlora/editor/Project.hpp"
#include "yqlora/editor/Selection.hpp"

#include <optional>

namespace yqlora {

class Component;
class Node;
class Scene;
class SceneSerializer;
class TypeInfo;

// Открытый в редакторе проект: его сцена и выделение. Без интерфейса,
// панели только вызывают эти методы.
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

    // Открывает проект и его главную сцену. Если сцена не читается, проект
    // всё равно открывается с пустой сценой, а метод возвращает false.
    bool open(const Project& project);
    void close();

    // Сохраняет сцену в файл главной сцены проекта.
    bool saveScene();

    // Сцену меняли после открытия или последнего сохранения.
    bool modified() const
    {
        return m_modified;
    }

    // Для правок, которые делаются в обход методов ниже (например, свойства в инспекторе).
    void markModified()
    {
        m_modified = true;
    }

    // nullptr, если такого узла в сцене нет.
    Node* findNode(NodeId id);
    Node* selectedNode();

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

private:
    Scene& m_scene;
    const SceneSerializer& m_serializer;
    std::optional<Project> m_project;
    Selection m_selection;
    bool m_modified = false;
};

} // namespace yqlora
