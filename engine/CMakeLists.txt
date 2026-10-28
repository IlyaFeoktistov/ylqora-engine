# Библиотека движка yqlora (.so или .dll): ядро, Application, рендереры,
# ImGui. Её подключают редактор, запуск игры и dll со скриптами, так что
# состояние движка (лог, локализация, контекст ImGui) у всех одно.

# Заголовки и настройки для кода, который собирается внутрь библиотеки:
# сам движок и статические бэкенды.
add_library(yqlora_engine_code INTERFACE)
target_include_directories(yqlora_engine_code INTERFACE include internal)
target_link_libraries(yqlora_engine_code INTERFACE glm::glm imgui_headers)
target_compile_definitions(yqlora_engine_code INTERFACE
    # Кватернионы и углы в радианах: единые правила для всего движка.
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
    YQLORA_BUILDING_ENGINE
    "IMGUI_API=${YQLORA_DLL_EXPORT}"
)

add_library(yqlora SHARED
    src/core/Application.cpp
    src/core/EngineServices.cpp
    src/core/FileFormat.cpp
    src/core/FrameClock.cpp
    src/core/Log.cpp
    src/core/Settings.cpp
    src/core/UiLayer.cpp
    src/core/Version.cpp
    src/i18n/Localization.cpp
    src/platform/Platform.cpp
    src/platform/Window.cpp
    src/platform/InputFeeder.cpp
    src/platform/MouseWrap.cpp
    src/input/Input.cpp
    src/scene/ExecutionOrder.cpp
    src/scene/GameTime.cpp
    src/scene/Node.cpp
    src/scene/NodeTree.cpp
    src/scene/Component.cpp
    src/scene/Transform3D.cpp
    src/scene/Node3D.cpp
    src/scene/Camera3D.cpp
    src/scene/PhaseList.cpp
    src/scene/Scene.cpp
    src/scene/SceneSerializer.cpp
    src/project/Project.cpp
    src/reflection/ComponentRules.cpp
    src/reflection/Property.cpp
    src/reflection/PropertyJson.cpp
    src/reflection/TypeRegistry.cpp
    src/render/FrameBuilder.cpp
    src/render/MeshData.cpp
    src/render/RendererFactory.cpp
    src/resource/ResourceDatabase.cpp
)
add_library(yqlora::yqlora ALIAS yqlora)

# include/ - публичный API для игр: математика, сцена, ввод, лог, tr().
# internal/ - окно, платформа, рендер, Application: см. yqlora::internal ниже.
target_include_directories(yqlora PUBLIC include PRIVATE src)
target_link_libraries(yqlora
    PUBLIC glm::glm imgui_headers
    PRIVATE yqlora_engine_code imgui SDL3::SDL3-static nlohmann_json::nlohmann_json
)
target_compile_definitions(yqlora PRIVATE YQLORA_ENGINE_VERSION="${PROJECT_VERSION}")
target_compile_definitions(yqlora INTERFACE
    GLM_FORCE_RADIANS
    GLM_ENABLE_EXPERIMENTAL
    "IMGUI_API=${YQLORA_DLL_IMPORT}"
)
if(UNIX AND NOT APPLE)
    # SDL помечает свои функции видимыми даже в статической сборке: прячем
    # всё, что пришло из статических архивов, наружу им незачем.
    target_link_options(yqlora PRIVATE "LINKER:--exclude-libs,ALL")
endif()
yqlora_set_warnings(yqlora)

# Доступ к внутренним заголовкам: для программы с редактором и тестов движка.
# Игровой код подключает только yqlora::yqlora.
add_library(yqlora_internal INTERFACE)
add_library(yqlora::internal ALIAS yqlora_internal)
target_include_directories(yqlora_internal INTERFACE internal)
# Внутренние заголовки построены на nlohmann/json (только заголовки).
target_link_libraries(yqlora_internal INTERFACE yqlora::yqlora nlohmann_json::nlohmann_json)
