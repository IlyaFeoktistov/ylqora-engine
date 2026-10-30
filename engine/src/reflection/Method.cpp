#include "yqlora/reflection/Method.hpp"

#include <cassert>

namespace yqlora {

namespace {

[[maybe_unused]] bool validParameters(std::span<const MethodParameter> parameters)
{
    bool defaults = false;
    for (const MethodParameter& parameter : parameters) {
        if (parameter.defaultValue) {
            if (static_cast<PropertyType>(parameter.defaultValue->index()) != parameter.type) {
                return false;
            }
            defaults = true;
        } else if (defaults) {
            return false;
        }
    }

    return true;
}

} // namespace

Method Method::forNode(
    std::string name,
    std::vector<MethodParameter> parameters,
    std::optional<PropertyType> result,
    NodeInvoker invoker
)
{
    assert(validParameters(parameters) && "Method: defaults must match the types and come last");
    Method method;
    method.m_name = std::move(name);
    method.m_parameters = std::move(parameters);
    method.m_result = result;
    method.m_nodeInvoke = std::move(invoker);

    return method;
}

Method Method::forComponent(
    std::string name,
    std::vector<MethodParameter> parameters,
    std::optional<PropertyType> result,
    ComponentInvoker invoker
)
{
    assert(validParameters(parameters) && "Method: defaults must match the types and come last");
    Method method;
    method.m_name = std::move(name);
    method.m_parameters = std::move(parameters);
    method.m_result = result;
    method.m_componentInvoke = std::move(invoker);

    return method;
}

std::optional<std::vector<PropertyValue>> Method::complete(std::span<const PropertyValue> arguments) const
{
    if (arguments.size() > m_parameters.size()) {
        return std::nullopt;
    }
    std::vector<PropertyValue> result(arguments.begin(), arguments.end());
    for (std::size_t i = 0; i < m_parameters.size(); ++i) {
        const MethodParameter& parameter = m_parameters[i];
        if (i >= arguments.size()) {
            if (!parameter.defaultValue) {
                return std::nullopt;
            }
            result.push_back(*parameter.defaultValue);
        } else if (static_cast<PropertyType>(arguments[i].index()) != parameter.type) {
            return std::nullopt;
        }
    }

    return result;
}

MethodCallResult Method::call(Node& node, std::span<const PropertyValue> arguments) const
{
    assert(m_nodeInvoke && "Method::call: this is a component method");
    const auto completed = complete(arguments);
    if (!completed) {
        return {};
    }

    return {true, m_nodeInvoke(node, *completed)};
}

MethodCallResult Method::call(Component& component, std::span<const PropertyValue> arguments) const
{
    assert(m_componentInvoke && "Method::call: this is a node method");
    const auto completed = complete(arguments);
    if (!completed) {
        return {};
    }

    return {true, m_componentInvoke(component, *completed)};
}

} // namespace yqlora
