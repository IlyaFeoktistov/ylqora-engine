#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/Component.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <string_view>

namespace yqlora {

// Компонент из файла, тип которого не зарегистрирован (модуль игры не собран,
// скрипт удалён). Хранит свой объект JSON ровно как прочитан и пишется обратно
// без изменений; в игре не участвует. Когда тип возвращается,
// SceneSerializer::reviveMissing заменяет заглушку настоящим компонентом.
//
// Своё имя типа у заглушки не пропавшее, а MissingComponent: иначе сцена
// отдала бы её в списке компонентов того типа, когда он вернётся.
class YQLORA_API MissingComponent final : public Component {
public:
    YQLORA_TYPE(MissingComponent, Component)

    // data - объект компонента из файла, с типом и свойствами.
    explicit MissingComponent(nlohmann::json data);

    // Имя пропавшего типа, как в файле.
    const std::string& missingType() const
    {
        return m_missingType;
    }

    const nlohmann::json& data() const
    {
        return m_data;
    }

private:
    nlohmann::json m_data;
    std::string m_missingType;
};

} // namespace yqlora
