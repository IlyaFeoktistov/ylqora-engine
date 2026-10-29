#include "yqlora/reflection/TypeBuilder.hpp"

namespace yqlora {

TypeBuilder& TypeBuilder::createNode(std::function<std::unique_ptr<Node>()> factory)
{
    assert(m_info.m_kind == TypeKind::Node && "createNode: the type is not a node");
    m_info.m_createNode = std::move(factory);

    return *this;
}

TypeBuilder& TypeBuilder::createComponent(std::function<std::unique_ptr<Component>()> factory)
{
    assertComponent();
    m_info.m_createComponent = std::move(factory);

    return *this;
}

TypeBuilder& TypeBuilder::createSystem(std::function<std::unique_ptr<SceneSystem>(Scene&)> factory)
{
    assert(m_info.m_kind == TypeKind::System && "createSystem: the type is not a scene system");
    m_info.m_createSystem = std::move(factory);

    return *this;
}

TypeBuilder& TypeBuilder::phases(LifecyclePhases phases)
{
    assertComponent();
    m_info.m_phases = phases;

    return *this;
}

} // namespace yqlora
