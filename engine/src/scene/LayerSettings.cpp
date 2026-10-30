#include "yqlora/scene/LayerSettings.hpp"

namespace yqlora {

LayerSettings::LayerSettings()
{
    setNames({});
}

void LayerSettings::setNames(std::vector<std::string> names)
{
    names.resize(kLayerCount);
    names.front() = kDefaultLayerName;
    m_names = std::move(names);
}

std::string_view LayerSettings::name(int layer) const
{
    return layer >= 0 && layer < kLayerCount ? std::string_view(m_names[static_cast<std::size_t>(layer)])
                                             : std::string_view();
}

} // namespace yqlora
