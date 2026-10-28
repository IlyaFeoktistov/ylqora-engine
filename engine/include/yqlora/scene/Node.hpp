#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/NodeId.hpp"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

class Scene;

// Базовый элемент дерева сцены. Родитель владеет детьми, ребёнок хранит
// невладеющий указатель на родителя. Имена и типы всегда на английском.
//
// Сам Node не имеет положения, это делают пространственные узлы (Node3D), у которых
// Transform есть всегда. Остальные компоненты подключаются по необходимости.
class YQLORA_API Node {
public:
    explicit Node(std::string name = "Node");
    virtual ~Node();

    Node(const Node&) = delete;
    Node& operator=(const Node&) = delete;

    // Корень иерархии: свой typeName без override, поэтому без YQLORA_TYPE.
    // Базы в реестре нет.
    using YqloraThisType = Node;
    using YqloraBaseType = void;

    static constexpr std::string_view staticTypeName()
    {
        return "Node";
    }

    virtual std::string_view typeName() const
    {
        return staticTypeName();
    }

    // Уникален и не меняется за жизнь узла: по нему узлы ссылаются друг
    // на друга в файлах сцен и в редакторе. Выдаётся случайно при создании.
    NodeId id() const
    {
        return m_id;
    }

    const std::string& name() const
    {
        return m_name;
    }

    void setName(std::string name)
    {
        m_name = std::move(name);
    }

    Node* parent() const
    {
        return m_parent;
    }

    // Сцена, в дереве которой узел. nullptr, пока узел вне сцены.
    Scene* scene() const
    {
        return m_scene;
    }

    std::span<const std::unique_ptr<Node>> children() const
    {
        return m_children;
    }

    // Создаёт узел типа T и делает его последним ребёнком.
    template <std::derived_from<Node> T, typename... Args>
    T& addChild(Args&&... args)
    {
        auto node = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *node;
        addChild(std::move(node));

        return ref;
    }

    Node& addChild(std::unique_ptr<Node> child);

    // Вставляет ребёнка на место index среди детей (не больше их числа),
    // например вернуть удалённый узел туда, где он был.
    Node& insertChild(std::size_t index, std::unique_ptr<Node> child);

    // Отцепляет ребёнка и возвращает владение им; nullptr, если это не наш ребёнок.
    std::unique_ptr<Node> removeChild(Node& child);

    // Поиск среди прямых детей по имени.
    Node* findChild(std::string_view name) const;

    std::span<const std::unique_ptr<Component>> components() const
    {
        return m_components;
    }

    // Создаёт компонент типа T и подключает к узлу.
    template <std::derived_from<Component> T, typename... Args>
    T& addComponent(Args&&... args)
    {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *component;
        addComponent(std::move(component));

        return ref;
    }

    // В сцене проверяет правила компонентов (ComponentRules.hpp): недостающие
    // обязательные компоненты добавляет перед ним сам, а нарушение правил
    // (не тот узел, второй такой же) - ошибка программы.
    Component& addComponent(std::unique_ptr<Component> component);

    // Вставляет компонент на место index в списке (не больше его длины).
    Component& insertComponent(std::size_t index, std::unique_ptr<Component> component);

    // Первый компонент типа T (или его наследника); nullptr, если такого нет.
    template <std::derived_from<Component> T>
    T* getComponent() const
    {
        for (const auto& c : m_components) {
            if (auto* typed = dynamic_cast<T*>(c.get())) {
                return typed;
            }
        }

        return nullptr;
    }

    // Компонент наш и не встроенный.
    bool canRemoveComponent(const Component& component) const;

    // Отключает компонент и возвращает владение им; nullptr, если снять нельзя
    // (см. canRemoveComponent).
    std::unique_ptr<Component> removeComponent(Component& component);

protected:
    // Для пространственных узлов (Node3D): подключает компонент, который нельзя снять.
    Component& addBuiltinComponent(std::unique_ptr<Component> component);

    virtual void onParentChanged()
    {
    }

    // Узел попал в дерево сцены или ушёл из него, см. Component::onEnterScene.
    // При входе сначала узел, потом дети; при выходе наоборот.
    virtual void onEnterScene()
    {
    }

    virtual void onExitScene()
    {
    }

private:
    // ID из файла сцены выставляет только загрузчик.
    friend class SceneSerializer;
    void setId(NodeId id);

    // Вход в сцену и выход и остановку вызывает сама сцена для корня,
    // остальное - узлы по дереву. Кадр игры идёт не по дереву, а по спискам
    // фаз сцены (Scene::advance).
    friend class Scene;
    void enterScene(Scene& scene);
    void exitScene();

    // Игра остановлена: onStop у компонентов узла и детей, которые успели
    // запуститься. Следующий update снова начнёт с onStart.
    void stop();

    void addRequiredComponents(const Component& component);

    NodeId m_id;
    std::string m_name;
    Node* m_parent = nullptr;
    Scene* m_scene = nullptr;
    std::vector<std::unique_ptr<Component>> m_components;
    std::vector<std::unique_ptr<Node>> m_children;
};

} // namespace yqlora
