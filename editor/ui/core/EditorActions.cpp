#include "core/EditorActions.hpp"

#include "core/ActionIds.hpp"
#include "core/Actions.hpp"

#include "yqlora/editor/EditorSession.hpp"

#include <imgui.h>

#include <utility>

namespace yqlora::editor {

void registerEditorActions(ActionRegistry& registry, EditorSession& session, EditorActionHandlers handlers)
{
    const auto hasProject = [&session] {
        return session.project().has_value();
    };
    const auto playing = [&session] {
        return session.playing();
    };

    registry.add(actions::kSaveScene, "editor.menu.save")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_S)
        // Во время игры в сцене её состояние, а не то, что правили: сохранять нечего.
        .enabledWhen([&session] {
            return session.project() && !session.playing();
        })
        .onRun(std::move(handlers.saveScene));
    registry.add(actions::kCloseProject, "editor.menu.closeProject")
        .enabledWhen(hasProject)
        .onRun(std::move(handlers.closeProject));
    registry.add(actions::kQuit, "editor.menu.quit").onRun(std::move(handlers.quit));

    registry.add(actions::kUndo, "editor.menu.undo")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_Z)
        .enabledWhen([&session] {
            return session.canUndo();
        })
        .onRun([&session] {
            session.undo();
        });
    registry.add(actions::kRedo, "editor.menu.redo")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_Y)
        .alternativeShortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_Z)
        .enabledWhen([&session] {
            return session.canRedo();
        })
        .onRun([&session] {
            session.redo();
        });

    registry.add(actions::kTogglePlay, "editor.play.play")
        .shortcut(ImGuiMod_Ctrl | ImGuiKey_P)
        .enabledWhen(hasProject)
        .onRun([&session] {
            if (session.playing()) {
                session.stop();
            } else {
                session.play();
            }
        });
    registry.add(actions::kPause, "editor.play.pause").enabledWhen(playing).onRun([&session] {
        session.setPaused(!session.playMode().paused());
    });
    registry.add(actions::kStop, "editor.play.stop").enabledWhen(playing).onRun([&session] {
        session.stop();
    });
}

} // namespace yqlora::editor
