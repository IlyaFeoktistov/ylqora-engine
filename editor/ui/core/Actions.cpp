#include "core/Actions.hpp"

#include "yqlora/i18n/Translate.hpp"

#include <imgui_internal.h>

#include <algorithm>
#include <cassert>

namespace yqlora {

namespace {

bool isEnabled(const Action& action)
{
    return !action.enabled || action.enabled();
}

} // namespace

void ActionRegistry::add(Action action)
{
    assert(!find(action.id) && "ActionRegistry::add: id is already taken");
    m_actions.push_back(std::move(action));
}

const Action* ActionRegistry::find(std::string_view id) const
{
    const auto it = std::ranges::find(m_actions, id, &Action::id);

    return it != m_actions.end() ? &*it : nullptr;
}

bool ActionRegistry::enabled(std::string_view id) const
{
    const Action* action = find(id);

    return action && isEnabled(*action);
}

void ActionRegistry::run(std::string_view id)
{
    const Action* action = find(id);
    if (action && isEnabled(*action)) {
        action->run();
    }
}

void ActionRegistry::handleShortcuts()
{
    for (const Action& action : m_actions) {
        const bool pressed = (action.shortcut != 0 && ImGui::Shortcut(action.shortcut, ImGuiInputFlags_RouteGlobal))
                             || (action.alternativeShortcut != 0
                                 && ImGui::Shortcut(action.alternativeShortcut, ImGuiInputFlags_RouteGlobal));
        if (pressed && isEnabled(action)) {
            action.run();
        }
    }
}

void ActionRegistry::menuItem(std::string_view id)
{
    const Action* action = find(id);
    if (!action) {
        return;
    }
    const std::string shortcut = shortcutText(id);
    if (ImGui::MenuItem(tr(action->labelKey).c_str(), shortcut.empty() ? nullptr : shortcut.c_str(), false,
                        isEnabled(*action))) {
        action->run();
    }
}

std::string ActionRegistry::shortcutText(std::string_view id) const
{
    const Action* action = find(id);
    if (!action || action->shortcut == 0) {
        return {};
    }

    return ImGui::GetKeyChordName(action->shortcut);
}

} // namespace yqlora
