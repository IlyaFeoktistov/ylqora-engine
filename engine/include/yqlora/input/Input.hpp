#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/input/Key.hpp"
#include "yqlora/math/Math.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>

namespace yqlora {

// Состояние клавиатуры и мыши на текущий кадр и действия ввода из настроек
// проекта (раздел Input). Только для чтения: заполняет его платформа, пока
// обрабатывает события окна.
class YQLORA_API Input {
public:
    // Клавиша зажата сейчас.
    bool isKeyDown(Key key) const;
    // Клавиша нажата или отпущена в этом кадре.
    bool wasKeyPressed(Key key) const;
    bool wasKeyReleased(Key key) const;

    bool isMouseButtonDown(MouseButton button) const;
    bool wasMouseButtonPressed(MouseButton button) const;
    bool wasMouseButtonReleased(MouseButton button) const;

    // Положение курсора в окне, в точках, от левого верхнего угла.
    Vec2 mousePosition() const;
    // Насколько сдвинулась мышь за кадр. Работает и при захваченном курсоре.
    Vec2 mouseDelta() const;
    // Прокрутка колеса за кадр: больше нуля - от себя.
    float wheelDelta() const;

    // Действия по имени (см. InputSettings). Неизвестное действие не нажато
    // и даёт 0.
    bool isActionDown(std::string_view action) const;
    bool wasActionPressed(std::string_view action) const;
    bool wasActionReleased(std::string_view action) const;
    // Ось: от -1 до 1. Кнопка: 1, пока зажата, иначе 0.
    float actionValue(std::string_view action) const;

private:
    friend class InputFeeder;

    static constexpr std::size_t kKeyCount = static_cast<std::size_t>(Key::Count);
    static constexpr std::size_t kButtonCount = static_cast<std::size_t>(MouseButton::Count);

    std::array<bool, kKeyCount> m_keysDown{};
    std::array<bool, kKeyCount> m_keysPressed{};
    std::array<bool, kKeyCount> m_keysReleased{};
    std::array<bool, kButtonCount> m_buttonsDown{};
    std::array<bool, kButtonCount> m_buttonsPressed{};
    std::array<bool, kButtonCount> m_buttonsReleased{};
    Vec2 m_mousePosition{0.0f};
    Vec2 m_mouseDelta{0.0f};
    float m_wheelDelta = 0.0f;

    struct ActionState {
        bool down = false;
        bool pressed = false;
        bool released = false;
        float value = 0.0f;
    };

    const ActionState* findAction(std::string_view action) const;

    std::map<std::string, ActionState, std::less<>> m_actions;
};

} // namespace yqlora
