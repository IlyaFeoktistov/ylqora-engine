#pragma once

#include <yqlora/core/EngineTypes.hpp>
#include <yqlora/reflection/TypeRegistry.hpp>

namespace yqlora::test {

// Реестр с типами движка, заполненный уже при создании. В фикстуре он должен
// быть готов раньше сцены: системы сцена берёт из реестра в конструкторе, а
// тело конструктора фикстуры выполняется после всех её полей.
struct EngineRegistry : TypeRegistry {
    EngineRegistry()
    {
        registerEngineTypes(*this);
    }
};

} // namespace yqlora::test
