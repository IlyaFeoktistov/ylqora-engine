cmake_minimum_required(VERSION 3.24)

project(yqlora VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Все исполняемые файлы и ресурсы складываем в одну папку, чтобы программа и тесты
# находили assets/ рядом с собой и на Linux, и на Windows (в т.ч. multi-config MSVC).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
foreach(config ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${config} config_upper)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} ${CMAKE_BINARY_DIR}/bin/${config})
endforeach()

option(YQLORA_BACKEND_OPENGL "Build the OpenGL renderer backend" ON)
option(YQLORA_BUILD_TESTS "Build unit tests" ON)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

add_subdirectory(third_party/glad)
add_subdirectory(engine)

if(YQLORA_BACKEND_OPENGL)
    add_subdirectory(backends/opengl)
endif()

# Итоговая библиотека движка: Application + фабрика рендереров + включённые бэкенды.
# Отдельная цель нужна, чтобы ядро не зависело от бэкендов (иначе зависимость циклическая).
add_library(yqlora STATIC
    engine/src/core/Application.cpp
    engine/src/render/RendererFactory.cpp
)
add_library(yqlora::yqlora ALIAS yqlora)
target_link_libraries(yqlora PUBLIC yqlora::internal PRIVATE imgui)
if(YQLORA_BACKEND_OPENGL)
    target_link_libraries(yqlora PRIVATE yqlora::opengl)
    target_compile_definitions(yqlora PRIVATE YQLORA_HAS_OPENGL=1)
endif()
yqlora_set_warnings(yqlora)

add_subdirectory(editor)

if(YQLORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
