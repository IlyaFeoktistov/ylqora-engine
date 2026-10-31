#pragma once

#include "yqlora/core/Export.hpp"

#include <nlohmann/json.hpp>

namespace yqlora {

class Component;

// Свойства из файла сцены, которых у типа компонента больше нет (поле убрали
// из кода): компонент хранит их как прочитаны, файл пишет обратно, поэтому
// вернуть поле можно без потерь. Ведёт их только сериализатор.
struct YQLORA_API UnknownProperties {
    // Объект JSON; пустой, если таких свойств нет.
    static nlohmann::json of(const Component& component);
    static void set(Component& component, const nlohmann::json& properties);
};

} // namespace yqlora
