#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"

#include <optional>

namespace yqlora {

// Перенос курсора к противоположному краю окна при обзоре камерой.
// Только расчёты, без SDL: окно само переносит курсор, а здесь решается,
// куда его переносить и какое смещение мыши засчитать.
class YQLORA_API MouseWrap {
public:
    // Начало переноса: положение, от которого считается смещение.
    void reset(const Vec2& position);

    // Смещение мыши для нового положения курсора. Скачок больше половины окна
    // за одно событие - след переноса курсора, а не движение руки: он не засчитывается.
    Vec2 delta(const Vec2& position, const Vec2& windowSize);

    // Куда перенести курсор, если он у края окна или уже за ним. nullopt, если не нужно.
    std::optional<Vec2> wrapTarget(const Vec2& position, const Vec2& windowSize) const;

    // Курсор перенесён: следующее смещение считается от новой точки.
    void warped(const Vec2& target);

    // Курсор после переноса раз за разом оказывается не там, куда его переносили:
    // система перенос не выполняет, хотя и не сообщает об ошибке (так в WSLg).
    bool warpIgnored() const
    {
        return m_missedWarps >= kMaxMissedWarps;
    }

private:
    // Пара промахов бывает и при рабочем переносе: из очереди приходят события,
    // отправленные до него.
    static constexpr int kMaxMissedWarps = 3;

    Vec2 m_lastPosition{0.0f};
    std::optional<Vec2> m_warpTarget;
    int m_missedWarps = 0;
};

} // namespace yqlora
