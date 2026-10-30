#include "yqlora/editor/ui/core/Actions.hpp"

#include "yqlora/i18n/Translate.hpp"

#include <imgui_internal.h>

#include <cassert>
#include <utility>

namespace yqlora::editor {

Action::Action(std::string id, std::string labelKey)
    : m_id(std::move(id))
    , m_labelKey(std::move(labelKey))
{
}

Action& Action::shortcut(ImGuiKeyChord chord)
{
    m_shortcut = chord;

    return *this;
}

Action& Action::alternativeShortcut(ImGuiKeyChord chord)
{
    m_alternativeShortcut = chord;

    return *this;
}

Action& Action::enabledWhen(std::function<bool()> condition)
{
    m_enabled = std::move(condition);

    return *this;
}

Action& Action::onRun(std::function<void()> run)
{
    m_run = std::move(run);

    return *this;
}

bool Action::enabled() const
{
    return !m_enabled || m_enabled();
}

void Action::run() const
{
    if (m_run && enabled()) {
        m_run();
    }
}

bool Action::shortcutPressed() const
{
    const auto pressed = [](ImGuiKeyChord chord) {
        return chord != 0 && ImGui::Shortcut(chord, ImGuiInputFlags_RouteGlobal);
    };

    return pressed(m_shortcut) || pressed(m_alternativeShortcut);
}

Action& ActionRegistry::add(std::string id, std::string labelKey)
{
    assert(!find(id) && "ActionRegistry::add: id is already taken");

    return *m_actions.emplace_back(std::make_unique<Action>(std::move(id), std::move(labelKey)));
}

const Action* ActionRegistry::find(std::string_view id) const
{
    for (const auto& action : m_actions) {
        if (action->id() == id) {
            return action.get();
        }
    }

    return nullptr;
}

bool ActionRegistry::enabled(std::string_view id) const
{
    const Action* action = find(id);

    return action && action->enabled();
}

void ActionRegistry::run(std::string_view id) const
{
    if (const Action* action = find(id)) {
        action->run();
    }
}

void ActionRegistry::handleShortcuts() const
{
    for (const auto& action : m_actions) {
        if (action->shortcutPressed()) {
            action->run();
        }
    }
}

void ActionRegistry::menuItem(std::string_view id) const
{
    const Action* action = find(id);
    if (!action) {
        return;
    }
    const std::string shortcut = shortcutText(id);
    const char* shortcutLabel = shortcut.empty() ? nullptr : shortcut.c_str();
    if (ImGui::MenuItem(tr(action->labelKey()).c_str(), shortcutLabel, false, action->enabled())) {
        action->run();
    }
}

std::string ActionRegistry::shortcutText(std::string_view id) const
{
    const Action* action = find(id);
    if (!action || action->shortcutChord() == 0) {
        return {};
    }

    return ImGui::GetKeyChordName(action->shortcutChord());
}

} // namespace yqlora::editor
