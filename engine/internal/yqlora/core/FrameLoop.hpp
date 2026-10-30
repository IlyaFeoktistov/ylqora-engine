#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/core/FrameClock.hpp"

namespace yqlora {

// Стадии одного кадра. Что делает каждая, решает владелец цикла (игра,
// редактор, тест), а порядок знает только FrameLoop.
class YQLORA_API FrameStages {
public:
    virtual ~FrameStages() = default;

    // События окна в сырой ввод. false - кадр дальше не идёт (окно закрывают).
    virtual bool input() = 0;
    // Шаг сцены: фиксированные шаги, update, late update, отложенные изменения дерева.
    virtual void simulate(float deltaSeconds) = 0;
    // Мировые матрицы изменённых узлов (Scene::updateTransforms).
    virtual void updateTransforms() = 0;
    // Снимок сцены для рендерера; после него сцену не читают.
    virtual void extract() = 0;
    virtual void render() = 0;
    // Интерфейс поверх кадра: ImGui редактора и инструментов.
    virtual void interface() = 0;
    virtual void present() = 0;
};

// Порядок кадра в одном месте, см. "Frame pipeline" в docs/architecture/frame.md.
// Длину кадра считает сам, по часам платформы.
class YQLORA_API FrameLoop {
public:
    FrameLoop(FrameStages& stages, double maxFrameSeconds);

    // Начинает отсчёт времени с момента nowSeconds.
    void reset(double nowSeconds);

    // Один кадр. false, если он прерван на вводе.
    bool runFrame(double nowSeconds);

private:
    FrameStages& m_stages;
    FrameClock m_clock;
};

} // namespace yqlora
