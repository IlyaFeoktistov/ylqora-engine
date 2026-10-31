# Собирает проект со скриптами из шаблона против SDK из сборки движка, теми
# же компиляторами и тем же CMakeLists.txt (sdk/game-project), что и
# редактор. Запуск: cmake -P с параметрами
#   TEMPLATE_DIR, BUILD_DIR, SDK_DIR, GENERATOR, MAKE_PROGRAM, CXX_COMPILER,
#   CXX_FLAGS, LINKER_FLAGS.

# Имя класса в шаблоне; так же его подставляет редактор (ProjectFiles::createScript).
set(placeholder "$SCRIPT_NAME$")

function(script_from_template folder name)
    foreach(extension hpp cpp)
        file(READ ${TEMPLATE_DIR}/Script.${extension} text)
        string(REPLACE "${placeholder}" "${name}" text "${text}")
        file(WRITE ${folder}/${name}.${extension} "${text}")
    endforeach()
endfunction()

set(project_dir ${BUILD_DIR}/project)
file(REMOVE_RECURSE ${BUILD_DIR})
script_from_template(${project_dir} Pulse)
script_from_template(${project_dir}/enemies Walker)
# Скрытые папки в сборку не попадают.
file(WRITE ${project_dir}/.hidden/Broken.cpp "this is not C++")

execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S ${SDK_DIR}/game-project
        -B ${BUILD_DIR}/build
        -G ${GENERATOR}
        -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}
        -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CXX_FLAGS}
        -DCMAKE_SHARED_LINKER_FLAGS=${LINKER_FLAGS}
        -Dyqlora_DIR=${SDK_DIR}/cmake
        -DYQLORA_PROJECT_DIR=${project_dir}
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Configuring the game project failed")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR}/build RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Building the game project failed")
endif()

file(READ ${BUILD_DIR}/build/yqlora_game_module.txt library)
if(NOT EXISTS "${library}")
    message(FATAL_ERROR "The recorded game module '${library}' does not exist")
endif()
