#pragma once

#include "yqlora/editor/Command.hpp"
#include "yqlora/editor/PlayMode.hpp"
#include "yqlora/editor/SceneDocument.hpp"
#include "yqlora/editor/SceneEditing.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Transform3D.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>

namespace yqlora {

class Component;
class EngineServices;
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
    // Открытый проект сессия ставит в services: его видят сцены редактора.
    EditorSession(EngineServices& services, const SceneSerializer& serializer);

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

    // Правка текущей сцены командами с отменой.
    SceneEditing& editing()
    {
        return m_editing;
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
    // input - ввод приложения, игра видит его, только пока окно игры в фокусе.
    void update(float deltaSeconds, const Input& input)
    {
        m_play.update(deltaSeconds, input);
        // Сцену документа правят между кадрами: мировые матрицы одним проходом.
        m_document.scene().updateTransforms();
    }

    void setGameInputFocused(bool focused)
    {
        m_play.setInputFocused(focused);
    }

    // nullptr, если такого узла в сцене нет.
    Node* findNode(NodeId id);
    Node* selectedNode();

private:
    // Правка ведёт историю документа.
    friend class SceneEditing;

    EngineServices& m_services;
    const SceneSerializer& m_serializer;
    std::optional<Project> m_project;
    SceneDocument m_document;
    PlayMode m_play;
    Selection m_selection;
    // Выделение до Play: к нему возвращаемся, если после игры выбранных узлов не осталось.
    Selection m_selectionBeforePlay;
    SceneEditing m_editing{*this};
};

// Уточнение заголовка окна: проект и открытая сцена, "*" у несохранённой.
// Пусто, если проект не открыт.
std::string windowTitle(const EditorSession& session);

} // namespace yqlora::editor
