include(FetchContent)

# Некоторые зависимости объявляют старый cmake_minimum_required, CMake 4.x без этого падает.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

# SDL3 собираем статически: на Windows не нужно таскать SDL3.dll рядом с exe.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SDL_STATIC_PIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.16
    GIT_SHALLOW TRUE
)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.2
    GIT_SHALLOW TRUE
)

# Всё качаем через git, а не URL: curl внутри некоторых сборок CMake (например, WinLibs)
# не видит системные сертификаты Windows, а git использует их всегда.
set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
    GIT_SHALLOW TRUE
)

# Dear ImGui (ветка docking) для интерфейса редактора. Своего CMakeLists у неё нет,
# поэтому цели собираем сами: ядро плюс платформенная часть на SDL3.
# Часть для конкретного графического API собирает соответствующий бэкенд.
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9b-docking
    GIT_SHALLOW TRUE
)

# ImGuizmo: манипуляторы перемещения, поворота и масштаба в окне сцены редактора.
# Её CMakeLists собирает лишнее, поэтому только скачиваем (SOURCE_SUBDIR без
# CMakeLists) и собираем один нужный файл сами.
FetchContent_Declare(imguizmo
    GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
    GIT_TAG 1.10
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR do-not-build
)

FetchContent_MakeAvailable(SDL3 glm nlohmann_json imgui imguizmo)

# ImGui и ImGuizmo входят в библиотеку движка, чтобы у всех модулей был один
# контекст ImGui. Внутри неё функции ImGui экспортируются, снаружи импортируются.
if(WIN32)
    set(YQLORA_DLL_EXPORT "__declspec(dllexport)")
    set(YQLORA_DLL_IMPORT "__declspec(dllimport)")
else()
    set(YQLORA_DLL_EXPORT "__attribute__((visibility(\"default\")))")
    set(YQLORA_DLL_IMPORT "__attribute__((visibility(\"default\")))")
endif()

# Только заголовки. SYSTEM: наши строгие предупреждения не распространяются на чужой код.
add_library(imgui_headers INTERFACE)
target_include_directories(imgui_headers SYSTEM INTERFACE
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${imgui_SOURCE_DIR}/misc/cpp
    ${imguizmo_SOURCE_DIR}/src
)

# OBJECT, а не STATIC: из статического архива в библиотеку попало бы только то,
# что вызывает сам движок, а редактору нужны и ImGuizmo, и обёртки над std::string.
add_library(imgui OBJECT
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
    ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
    ${imguizmo_SOURCE_DIR}/src/ImGuizmo.cpp
)
target_link_libraries(imgui PUBLIC imgui_headers PRIVATE SDL3::SDL3-static)
target_compile_definitions(imgui PRIVATE "IMGUI_API=${YQLORA_DLL_EXPORT}")

if(YQLORA_BUILD_TESTS)
    FetchContent_Declare(doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG v2.4.12
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(doctest)
endif()
