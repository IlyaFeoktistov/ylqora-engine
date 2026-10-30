#pragma once

#include "yqlora/core/Export.hpp"

#include <functional>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

class Component;

// Компоненты сцены по имени типа, в порядке входа в сцену. По имени, а не по
// C++-типу: у типов, объявленных во время выполнения, свои списки.
class YQLORA_API ComponentIndex {
public:
    void add(Component& component);
    void remove(Component& component);

    std::span<Component* const> ofType(std::string_view typeName) const;

private:
    std::map<std::string, std::vector<Component*>, std::less<>> m_byType;
};

} // namespace yqlora
