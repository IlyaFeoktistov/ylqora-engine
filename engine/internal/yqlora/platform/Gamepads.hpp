#pragma once

#include "yqlora/core/Export.hpp"

#include <cstdint>
#include <vector>

union SDL_Event;

namespace yqlora {

class InputFeeder;

// Подключённые геймпады. Игрок один: его геймпад - первый подключённый; когда
// его отключают, кнопки отпускаются и его место занимает следующий.
// Принадлежит приложению, а не окну: окно пересоздают при смене графического
// API, а подключённые геймпады это переживают.
class YQLORA_API Gamepads {
public:
    // Включает геймпады в SDL; уже подключённые придут событиями подключения.
    Gamepads();
    ~Gamepads();

    Gamepads(const Gamepads&) = delete;
    Gamepads& operator=(const Gamepads&) = delete;

    // Событие геймпада в ввод. false, если событие не про геймпад.
    bool handleEvent(const SDL_Event& event, InputFeeder& input);

private:
    void connected(std::uint32_t id, InputFeeder& input);
    void disconnected(std::uint32_t id, InputFeeder& input);

    std::vector<std::uint32_t> m_open;
    // 0 - геймпада у игрока нет.
    std::uint32_t m_active = 0;
    bool m_initialized = false;
};

} // namespace yqlora
