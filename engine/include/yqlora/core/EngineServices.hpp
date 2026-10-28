#pragma once

#include "yqlora/core/Export.hpp"

namespace yqlora {

class Input;
class Project;
class TypeRegistry;

// Службы движка, общие для всех сцен процесса. Их даёт сцене её владелец:
// игра, редактор или тест. Глобальной точки доступа нет: компонент берёт
// службы у своей сцены.
class YQLORA_API EngineServices {
public:
    EngineServices(const Input& input, const TypeRegistry& registry);

    // Пустые службы: ничего не нажато, реестр без типов, проекта нет. Для сцен,
    // которые только читают и пишут файлы. В игре компоненты такой сцены не
    // вызываются: без реестра сцена не знает их фаз.
    static const EngineServices& none();

    const Input& input() const
    {
        return m_input;
    }

    const TypeRegistry& registry() const
    {
        return m_registry;
    }

    // nullptr, пока проект не открыт.
    const Project* project() const
    {
        return m_project;
    }

    void setProject(const Project* project)
    {
        m_project = project;
    }

private:
    const Input& m_input;
    const TypeRegistry& m_registry;
    const Project* m_project = nullptr;
};

} // namespace yqlora
