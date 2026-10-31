# Подключается из engine/: копии после сборки вешаются на цель yqlora там,
# где она объявлена.
#
# SDK рядом с библиотекой движка (bin/sdk/): по нему проект собирает свой
# модуль игры через find_package(yqlora). Пути внутри пакета вычисляются от
# его места, поэтому перенесённый вместе с редактором SDK работает.
#
#   sdk/include/        публичные заголовки движка и glm
#   sdk/lib/            библиотека импорта (Windows)
#   sdk/cmake/          yqloraConfig.cmake, версия, yqlora_add_game_module
#                       и входная функция модуля
#   sdk/game-project/   CMakeLists.txt модуля игры проекта: его настраивает редактор
#   sdk/toolchain.json  набор инструментов, которым собран движок

include(CMakePackageConfigHelpers)

set(YQLORA_SDK_DIR $<TARGET_FILE_DIR:yqlora>/sdk)
set(YQLORA_SDK_STAGING ${CMAKE_BINARY_DIR}/sdk-staging)

# Библиотека времени выполнения MSVC, которой собран движок: модуль игры
# обязан взять ту же. Пустое свойство - умолчание CMake.
set(YQLORA_MSVC_RUNTIME "")
if(MSVC)
    get_target_property(YQLORA_MSVC_RUNTIME yqlora MSVC_RUNTIME_LIBRARY)
    if(NOT YQLORA_MSVC_RUNTIME)
        set(YQLORA_MSVC_RUNTIME "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
endif()

# vcvarsall.bat поднимает окружение Visual Studio для сборки проекта. cl.exe
# лежит в VC/Tools/MSVC/<версия>/bin/Host<arch>/<arch>/, vcvarsall - в
# VC/Auxiliary/Build/.
set(YQLORA_VCVARSALL "")
if(MSVC)
    set(levels_from_cl_to_vc 6)
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH vc_dir)
    foreach(level RANGE 1 ${levels_from_cl_to_vc})
        cmake_path(GET vc_dir PARENT_PATH vc_dir)
    endforeach()
    set(YQLORA_VCVARSALL ${vc_dir}/Auxiliary/Build/vcvarsall.bat)
endif()

configure_file(${PROJECT_SOURCE_DIR}/cmake/sdk/yqloraConfig.cmake.in ${YQLORA_SDK_STAGING}/yqloraConfig.cmake @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/cmake/sdk/toolchain.json.in ${YQLORA_SDK_STAGING}/toolchain.json @ONLY)
# Модуль игры собирается ровно под эту версию движка: ABI между версиями не обещан.
write_basic_package_version_file(
    ${YQLORA_SDK_STAGING}/yqloraConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY ExactVersion
)

# Шаблоны содержат выражения генератора (имена файлов библиотеки, библиотека
# времени выполнения): они раскрываются для каждой конфигурации.
file(GENERATE OUTPUT ${YQLORA_SDK_DIR}/cmake/yqloraConfig.cmake INPUT ${YQLORA_SDK_STAGING}/yqloraConfig.cmake)
file(GENERATE OUTPUT ${YQLORA_SDK_DIR}/cmake/yqloraConfigVersion.cmake
    INPUT ${YQLORA_SDK_STAGING}/yqloraConfigVersion.cmake
)
file(GENERATE OUTPUT ${YQLORA_SDK_DIR}/toolchain.json INPUT ${YQLORA_SDK_STAGING}/toolchain.json)

# Копии сверяются при каждой сборке (цель всегда устаревшая), а не только
# когда перелинкована библиотека: правка одного CMake-файла SDK её не
# перелинковывает. if_different не трогает неизменённые файлы, иначе каждая
# сборка движка пересобирала бы все скрипты проекта.
set(sdk_copy
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        ${PROJECT_SOURCE_DIR}/engine/include ${YQLORA_SDK_DIR}/include
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different ${glm_SOURCE_DIR}/glm ${YQLORA_SDK_DIR}/include/glm
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/cmake/YqloraGameModule.cmake ${YQLORA_SDK_DIR}/cmake/YqloraGameModule.cmake
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/cmake/GameModuleEntry.cpp ${YQLORA_SDK_DIR}/cmake/GameModuleEntry.cpp
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/cmake/game-project/CMakeLists.txt ${YQLORA_SDK_DIR}/game-project/CMakeLists.txt
)
if(WIN32)
    list(APPEND sdk_copy
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_LINKER_FILE:yqlora> ${YQLORA_SDK_DIR}/lib/$<TARGET_LINKER_FILE_NAME:yqlora>
    )
endif()
add_custom_target(yqlora_sdk ALL ${sdk_copy} VERBATIM)
add_dependencies(yqlora_sdk yqlora)
