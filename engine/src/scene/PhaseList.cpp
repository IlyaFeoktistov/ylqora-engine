#include "yqlora/scene/PhaseList.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

namespace yqlora {

void PhaseList::add(Component& component, int executionOrder)
{
    Entry entry{.component = &component, .order = executionOrder, .sequence = m_nextSequence++};
    if (m_iterating) {
        m_pending.push_back(entry);

        return;
    }
    insert(entry);
}

void PhaseList::remove(Component& component)
{
    const auto matches = [&](const Entry& entry) {
        return entry.component == &component;
    };
    std::erase_if(m_pending, matches);
    if (m_iterating) {
        for (Entry& entry : m_entries) {
            if (matches(entry)) {
                entry.component = nullptr;
            }
        }

        return;
    }
    std::erase_if(m_entries, matches);
}

void PhaseList::reorder(const std::function<int(const Component&)>& executionOrder)
{
    assert(!m_iterating && "PhaseList::reorder: not during forEach");
    for (Entry& entry : m_entries) {
        entry.order = executionOrder(*entry.component);
    }
    std::ranges::sort(m_entries, [](const Entry& a, const Entry& b) {
        return a.order != b.order ? a.order < b.order : a.sequence < b.sequence;
    });
}

void PhaseList::forEach(const std::function<void(Component&)>& call)
{
    m_iterating = true;
    // По индексу: вызов может добавить компонент, а добавленные ждут в m_pending.
    for (std::size_t i = 0; i < m_entries.size(); ++i) {
        if (Component* component = m_entries[i].component) {
            call(*component);
        }
    }
    m_iterating = false;

    std::erase_if(m_entries, [](const Entry& entry) {
        return entry.component == nullptr;
    });
    for (const Entry& entry : std::exchange(m_pending, {})) {
        insert(entry);
    }
}

bool PhaseList::contains(const Component& component) const
{
    const auto matches = [&](const Entry& entry) {
        return entry.component == &component;
    };

    return std::ranges::any_of(m_entries, matches) || std::ranges::any_of(m_pending, matches);
}

std::size_t PhaseList::size() const
{
    return static_cast<std::size_t>(std::ranges::count_if(
               m_entries,
               [](const Entry& entry) {
                   return entry.component != nullptr;
               }
           ))
           + m_pending.size();
}

void PhaseList::insert(Entry entry)
{
    // После всех с тем же порядком: при равном порядке раньше тот, кто раньше добавлен.
    const auto at = std::ranges::upper_bound(m_entries, entry.order, std::less<>{}, &Entry::order);
    m_entries.insert(at, entry);
}

} // namespace yqlora
