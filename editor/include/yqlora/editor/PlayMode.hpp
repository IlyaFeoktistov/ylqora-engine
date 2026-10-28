#pragma once

#include <memory>

namespace yqlora {

class Scene;
class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Игра в редакторе: копия редактируемой сцены, которая живёт от Play до Stop.
// Сама редактируемая сцена во время игры не меняется.
class PlayMode {
public:
    explicit PlayMode(const SceneSerializer& serializer);
    ~PlayMode();

    PlayMode(const PlayMode&) = delete;
    PlayMode& operator=(const PlayMode&) = delete;

    // Копирует source и запускает игру в копии. false, если игра уже идёт
    // или копию сделать не удалось.
    bool start(const Scene& source);

    // Останавливает игру и выбрасывает копию.
    void stop();

    void setPaused(bool paused);

    bool playing() const
    {
        return m_scene != nullptr;
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

    void update(float deltaSeconds);

    // Копия для игры; nullptr, пока игры нет.
    Scene* scene()
    {
        return m_scene.get();
    }

    const Scene* scene() const
    {
        return m_scene.get();
    }

private:
    const SceneSerializer& m_serializer;
    std::unique_ptr<Scene> m_scene;
    bool m_paused = false;
};

} // namespace yqlora::editor
