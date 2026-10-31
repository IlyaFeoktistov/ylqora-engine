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

# Движок - разделяемая библиотека, и всё статическое, что входит в неё
# (SDL, ImGui, бэкенды), собирается как позиционно-независимый код.
# Символы по умолчанию скрыты: наружу видно только помеченное YQLORA_API и IMGUI_API.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(YQLORA_BACKEND_OPENGL "Build the OpenGL renderer backend" ON)
option(YQLORA_BUILD_TESTS "Build unit tests" ON)
option(YQLORA_BUILD_EXAMPLES "Build the example game modules" ON)

include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

add_subdirectory(third_party/glad)
add_subdirectory(engine)

if(YQLORA_BACKEND_OPENGL)
    add_subdirectory(backends/opengl)
endif()

# Бэкенды собираются статически внутрь библиотеки движка.
if(YQLORA_BACKEND_OPENGL)
    target_link_libraries(yqlora PRIVATE yqlora_opengl)
    target_compile_definitions(yqlora PRIVATE YQLORA_HAS_OPENGL=1)
endif()

add_subdirectory(editor)

# Программы. Сейчас только редактор, позже рядом встанет запуск игры без редактора.
add_subdirectory(apps/editor)

if(YQLORA_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(YQLORA_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
