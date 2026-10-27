# Вшивает файлы шейдеров в заголовок C++: каждый файл становится строковой
# константой в yqlora::shaders, имя из имени файла (outline_mask.frag -> kOutlineMaskFrag).
# Запускается при сборке: cmake -DOUTPUT=<header> -DSHADERS=<файлы через |> -P EmbedShaders.cmake
# Так исходники шейдеров лежат отдельными файлами, а собранной программе они не нужны на диске.

string(REPLACE "|" ";" SHADERS "${SHADERS}")
set(DELIMITER "yqlora_shader")
set(CONTENT "// Сгенерировано cmake/EmbedShaders.cmake из файлов шейдеров, не править руками.\n#pragma once\n\nnamespace yqlora::shaders {\n")

foreach(SHADER ${SHADERS})
    get_filename_component(BASE ${SHADER} NAME_WE)
    get_filename_component(EXTENSION ${SHADER} LAST_EXT)
    string(SUBSTRING ${EXTENSION} 1 -1 EXTENSION)

    set(NAME "k")
    string(REPLACE "_" ";" PARTS "${BASE};${EXTENSION}")
    foreach(PART ${PARTS})
        string(SUBSTRING ${PART} 0 1 FIRST)
        string(SUBSTRING ${PART} 1 -1 REST)
        string(TOUPPER ${FIRST} FIRST)
        string(APPEND NAME "${FIRST}${REST}")
    endforeach()

    file(READ ${SHADER} SOURCE)
    string(FIND "${SOURCE}" ")${DELIMITER}\"" CLASH)
    if(NOT CLASH EQUAL -1)
        message(FATAL_ERROR "Shader ${SHADER} contains the raw string delimiter")
    endif()
    string(APPEND CONTENT "\ninline constexpr const char* ${NAME} = R\"${DELIMITER}(${SOURCE})${DELIMITER}\";\n")
endforeach()

string(APPEND CONTENT "\n} // namespace yqlora::shaders\n")

# Пишем, только если поменялось: иначе пересобирались бы все, кто его подключает.
if(EXISTS ${OUTPUT})
    file(READ ${OUTPUT} OLD)
endif()
if(NOT "${OLD}" STREQUAL "${CONTENT}")
    file(WRITE ${OUTPUT} "${CONTENT}")
endif()
