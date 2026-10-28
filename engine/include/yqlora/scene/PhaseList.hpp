#pragma once

#include "yqlora/core/Export.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace yqlora {

class Component;

// Компоненты одной фазы кадра в порядке вызова: по порядку выполнения типа
// (меньше - раньше), при равном - в порядке добавления. Менять можно и во
// время обхода: добавленный вызывается со следующего обхода, снятый
// пропускается сразу.
class YQLORA_API PhaseList {
public:
    void add(Component& component, int executionOrder);
    void remove(Component& component);

    // Вызывает call для каждого компонента по порядку.
    void forEach(const std::function<void(Component&)>& call);

    bool contains(const Component& component) const;

    std::size_t size() const;

private:
    struct Entry {
        // nullptr - сняли во время обхода, убирается после него.
        Component* component = nullptr;
        int order = 0;
        std::uint64_t sequence = 0;
    };

    void insert(Entry entry);

    std::vector<Entry> m_entries;
    std::vector<Entry> m_pending;
    std::uint64_t m_nextSequence = 0;
    bool m_iterating = false;
};

} // namespace yqlora
