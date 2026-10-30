#include "yqlora/scene/ExecutionOrderSettings.hpp"

#include <algorithm>

namespace yqlora {

std::optional<int> ExecutionOrderSettings::orderOf(std::string_view type) const
{
    const auto it = std::ranges::find(entries, type, &ExecutionOrderEntry::type);

    return it != entries.end() ? std::optional(it->order) : std::nullopt;
}

} // namespace yqlora
