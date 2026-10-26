#pragma once

#include "yqlora/input/Input.hpp"

namespace yqlora {

// Единственный, кто пишет в Input: окно передаёт сюда события ОС.
// Отдельный класс нужен, чтобы запись не попала в публичный API игры.
class InputFeeder {
public:
    explicit InputFeeder(Input& input)
        : m_input(input)
    {
    }

    // Начало кадра: сбрасывает всё, что относится только к прошлому кадру.
    void beginFrame();

    void keyDown(Key key);
    void keyUp(Key key);
    void mouseButtonDown(MouseButton button);
    void mouseButtonUp(MouseButton button);
    void mouseMoved(const Vec2& position, const Vec2& delta);
    void wheelScrolled(float delta);

    // Окно потеряло фокус: отпускаем всё, иначе клавиша, отпущенная
    // в другом окне, так и осталась бы зажатой.
    void releaseAll();

private:
    Input& m_input;
};

} // namespace yqlora
