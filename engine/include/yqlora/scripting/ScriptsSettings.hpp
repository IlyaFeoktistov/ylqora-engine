#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/project/SettingsSection.hpp"

#include <array>
#include <string_view>

namespace yqlora {

// Как собирать модуль игры. Меняет только оптимизацию и отладочные сведения:
// библиотека времени выполнения всегда та же, что у движка.
enum class ScriptConfiguration {
    Debug,
    Release,
};

// Имена по порядку ScriptConfiguration: в файле настроек, в редакторе и как
// CMAKE_BUILD_TYPE.
inline constexpr std::array<std::string_view, 2> kScriptConfigurationNames{"Debug", "Release"};

// Раздел Scripts настроек проекта: сборка модуля игры из папки scripts/.
class YQLORA_API ScriptsSettings final : public SettingsSection {
public:
    YQLORA_TYPE(ScriptsSettings, SettingsSection)

    ScriptConfiguration configuration = ScriptConfiguration::Debug;
};

} // namespace yqlora
