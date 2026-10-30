#pragma once

#include "yqlora/editor/ui/core/EditorPanel.hpp"
#include "yqlora/editor/ui/widgets/RenderView.hpp"

namespace yqlora::editor {

// Окно игры: сцена через её главную камеру, как её увидит игрок. Во время
// Play - копия Play, без игры - редактируемая сцена. Пока окно в фокусе,
// игра видит ввод. Окно вида рядом остаётся с камерой редактора, в нём
// можно править копию Play, пока игра идёт.
class GamePanel final : public EditorPanel {
public:
    const char* id() const override
    {
        return "Game";
    }

    const char* titleKey() const override
    {
        return "editor.window.game";
    }

    // Без окна игры Play некуда показывать.
    bool closable() const override
    {
        return false;
    }

    void draw(EditorContext& context) override;

private:
    RenderView m_view;
    bool m_wasPlaying = false;
};

} // namespace yqlora::editor
