#include "yqlora/reflection/TypeInfo.hpp"

#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/scene/Component.hpp"
#include "yqlora/scene/Node.hpp"
#include "yqlora/scene/SceneSystem.hpp"

namespace yqlora {

namespace {

// Ключ перевода: type.<имя типа>.description.
constexpr std::string_view kDescriptionKeyPrefix = "type.";
constexpr std::string_view kDescriptionKeySuffix = ".description";

} // namespace

std::string TypeInfo::descriptionKey() const
{
    std::string key;
    key.reserve(kDescriptionKeyPrefix.size() + m_name.size() + kDescriptionKeySuffix.size());
    key.append(kDescriptionKeyPrefix).append(m_name).append(kDescriptionKeySuffix);

    return key;
}

std::unique_ptr<Node> TypeInfo::createNode() const
{
    return m_createNode ? m_createNode() : nullptr;
}

std::unique_ptr<Component> TypeInfo::createComponent() const
{
    return m_createComponent ? m_createComponent() : nullptr;
}

std::unique_ptr<SceneSystem> TypeInfo::createSystem(Scene& scene) const
{
    return m_createSystem ? m_createSystem(scene) : nullptr;
}

std::unique_ptr<SettingsSection> TypeInfo::createSettings() const
{
    return m_createSettings ? m_createSettings() : nullptr;
}

} // namespace yqlora
