#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/render/GraphicsApi.hpp"

namespace yqlora {

class EngineServices;

// Раздел Graphics настроек проекта. Применяется сразу: смена API пересоздаёт
// только рендер (окно, если API нужно другое, рендерер, отрисовку
// интерфейса), а не всё приложение; vsync переключается без пересоздания.
class YQLORA_API GraphicsSettings final : public SettingsSection {
public:
    YQLORA_TYPE(GraphicsSettings, SettingsSection)

    GraphicsApi windowsApi = GraphicsApi::OpenGL;
    GraphicsApi linuxApi = GraphicsApi::OpenGL;
    // Не рисовать кадры быстрее, чем обновляется экран.
    bool vsync = true;

    // API для системы, на которой работает программа.
    GraphicsApi currentPlatformApi() const;
};

// Раздел Graphics открытого проекта, а без проекта - значения по умолчанию.
YQLORA_API const GraphicsSettings& graphicsSettingsOf(const EngineServices& services);

} // namespace yqlora
