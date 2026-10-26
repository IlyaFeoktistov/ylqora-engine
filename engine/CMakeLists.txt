# Ядро движка: сцена, окно, локализация, настройки, интерфейс рендерера.
# Ничего не знает о конкретных графических API.
add_library(yqlora_core STATIC
    src/core/Log.cpp
    src/core/Settings.cpp
    src/i18n/Localization.cpp
    src/platform/Platform.cpp
    src/platform/Window.cpp
    src/platform/InputFeeder.cpp
    src/input/Input.cpp
    src/scene/Node.cpp
    src/scene/Component.cpp
    src/scene/Transform2D.cpp
    src/scene/Transform3D.cpp
    src/scene/Node2D.cpp
    src/scene/Node3D.cpp
    src/scene/Camera3D.cpp
    src/scene/MeshInstance3D.cpp
    src/scene/Scene.cpp
    src/scene/SceneSerializer.cpp
    src/reflection/Property.cpp
    src/reflection/TypeRegistry.cpp
    src/render/FrameBuilder.cpp
    src/render/MeshData.cpp
)
add_library(yqlora::core ALIAS yqlora_core)

# include/ - публичный API для игр: математика, сцена, ввод, лог, tr().
# internal/ - окно, платформа, рендер, Application. Игре недоступно.
target_include_directories(yqlora_core
    PUBLIC include
    PRIVATE internal
)
target_link_libraries(yqlora_core
    PUBLIC glm::glm
    PRIVATE SDL3::SDL3-static nlohmann_json::nlohmann_json
)
# Кватернионы и углы в радианах: единые правила для всего движка.
target_compile_definitions(yqlora_core PUBLIC GLM_FORCE_RADIANS GLM_ENABLE_EXPERIMENTAL)
yqlora_set_warnings(yqlora_core)

# Доступ к внутренним заголовкам: для бэкендов, хост-приложений (sandbox,
# будущий редактор) и тестов движка. Игровой код подключает только yqlora::core.
add_library(yqlora_internal INTERFACE)
add_library(yqlora::internal ALIAS yqlora_internal)
target_include_directories(yqlora_internal INTERFACE internal)
target_link_libraries(yqlora_internal INTERFACE yqlora::core)
