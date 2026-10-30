#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/reflection/Property.hpp"

#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace yqlora {

class Node;
class Component;

// Параметр метода. Значение по умолчанию есть только у последних параметров
// подряд: такие аргументы при вызове можно не передавать.
struct MethodParameter {
    std::string name;
    PropertyType type = PropertyType::Bool;
    std::optional<PropertyValue> defaultValue;
};

// Итог Method::call.
struct MethodCallResult {
    // false: аргументы не подошли под параметры, метод не вызван.
    bool called = false;
    // Пусто у метода без результата.
    std::optional<PropertyValue> value;
};

// Метод узла или компонента, который можно вызвать по имени: из Luau, из
// визуального скрипта. Как и Property, это функция над PropertyValue, поэтому
// метод может объявить и C++-класс (ClassTypeBuilder::method), и тип,
// описанный во время выполнения.
class YQLORA_API Method {
public:
    // Аргументы уже проверены и дополнены значениями по умолчанию.
    using NodeInvoker = std::function<std::optional<PropertyValue>(Node&, std::span<const PropertyValue>)>;
    using ComponentInvoker = std::function<std::optional<PropertyValue>(Component&, std::span<const PropertyValue>)>;

    // result - тип результата, nullopt у метода без результата.
    static Method forNode(
        std::string name,
        std::vector<MethodParameter> parameters,
        std::optional<PropertyType> result,
        NodeInvoker invoker
    );
    static Method forComponent(
        std::string name,
        std::vector<MethodParameter> parameters,
        std::optional<PropertyType> result,
        ComponentInvoker invoker
    );

    const std::string& name() const
    {
        return m_name;
    }

    std::span<const MethodParameter> parameters() const
    {
        return m_parameters;
    }

    const std::optional<PropertyType>& result() const
    {
        return m_result;
    }

    // Объект должен быть того типа, для которого зарегистрирован метод, или его наследником.
    MethodCallResult call(Node& node, std::span<const PropertyValue> arguments) const;
    MethodCallResult call(Component& component, std::span<const PropertyValue> arguments) const;

private:
    Method() = default;

    // Аргументы с недостающими значениями по умолчанию; nullopt, если не подходят.
    std::optional<std::vector<PropertyValue>> complete(std::span<const PropertyValue> arguments) const;

    std::string m_name;
    std::vector<MethodParameter> m_parameters;
    std::optional<PropertyType> m_result;
    // Своё у узлов и у компонентов, как в Property.
    NodeInvoker m_nodeInvoke;
    ComponentInvoker m_componentInvoke;
};

} // namespace yqlora
