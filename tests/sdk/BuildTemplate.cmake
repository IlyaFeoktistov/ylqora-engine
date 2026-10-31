# Собирает шаблон scripts/ нового проекта против SDK из сборки движка, теми
# же компиляторами, как это сделает редактор. Запуск: cmake -P с параметрами
#   TEMPLATE_DIR, BUILD_DIR, SDK_DIR, GENERATOR, MAKE_PROGRAM, CXX_COMPILER,
#   CXX_FLAGS, LINKER_FLAGS.

file(REMOVE_RECURSE ${BUILD_DIR})
execute_process(
    COMMAND ${CMAKE_COMMAND}
        -S ${TEMPLATE_DIR}
        -B ${BUILD_DIR}
        -G ${GENERATOR}
        -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}
        -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
        -DCMAKE_CXX_FLAGS=${CXX_FLAGS}
        -DCMAKE_SHARED_LINKER_FLAGS=${LINKER_FLAGS}
        -Dyqlora_DIR=${SDK_DIR}/cmake
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Configuring the scripts template failed")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "Building the scripts template failed")
endif()

file(READ ${BUILD_DIR}/yqlora_game_module.txt library)
if(NOT EXISTS "${library}")
    message(FATAL_ERROR "The recorded game module '${library}' does not exist")
endif()
