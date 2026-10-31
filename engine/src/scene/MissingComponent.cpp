#include "yqlora/scene/MissingComponent.hpp"

#include <utility>

namespace yqlora {

namespace {

constexpr const char* kTypeKey = "type";

} // namespace

MissingComponent::MissingComponent(nlohmann::json data)
    : m_data(std::move(data))
    , m_missingType(m_data.value(kTypeKey, std::string()))
{
}

} // namespace yqlora
