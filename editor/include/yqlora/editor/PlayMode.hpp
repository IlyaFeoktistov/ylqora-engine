#pragma once

#include "yqlora/input/Input.hpp"

#include <memory>

namespace yqlora {

class EngineServices;
class Scene;
class SceneSerializer;

} // namespace yqlora

namespace yqlora::editor {

// Игра в редакторе: копия редактируемой сцены, которая живёт от Play до Stop.
// Сама редактируемая сцена во время игры не меняется. У копии свой ввод: она
// видит нажатия, только пока окно игры в фокусе, так что клавиши, набранные
// в инспекторе, не двигают игрока.
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

    // Окно игры в фокусе: со следующего кадра игра видит ввод.
    void setInputFocused(bool focused)
    {
        m_inputFocused = focused;
    }

    // input - ввод приложения за этот кадр.
    void update(float deltaSeconds, const Input& input);

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
    // Что видит игра: ввод приложения или, без фокуса, ничего не нажато.
    Input m_input;
    // Службы копии: как у редактируемой сцены, но со своим вводом. Живут
    // дольше сцены, она хранит ссылку на них.
    std::unique_ptr<EngineServices> m_services;
    std::unique_ptr<Scene> m_scene;
    bool m_paused = false;
    bool m_inputFocused = false;
};

} // namespace yqlora::editor
