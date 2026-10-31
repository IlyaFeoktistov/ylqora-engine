# Модуль игры: разделяемая библиотека со скриптами, которую загружает движок.
# Один файл правил и для примеров в дереве движка, и для проектов через SDK
# (find_package(yqlora) подключает его копию).
#
#   yqlora_add_game_module(game Module.cpp Player.cpp)
function(yqlora_add_game_module target)
    add_library(${target} SHARED ${ARGN})
    target_link_libraries(${target} PRIVATE yqlora::yqlora)
    set_target_properties(${target} PROPERTIES
        OUTPUT_NAME game
        # Наружу видна только входная функция (YQLORA_MODULE_ENTRY).
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
    if(WIN32)
        # MinGW по умолчанию дал бы libgame.dll.
        set_target_properties(${target} PROPERTIES PREFIX "")
    endif()
    if(MSVC)
        # Заголовки движка в UTF-8.
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
    # Библиотека времени выполнения MSVC - та же, что у движка, иначе у них
    # разные кучи и разные std::string. Конфигурация её не меняет.
    if(DEFINED YQLORA_MSVC_RUNTIME_LIBRARY)
        set_target_properties(${target} PROPERTIES MSVC_RUNTIME_LIBRARY "${YQLORA_MSVC_RUNTIME_LIBRARY}")
    endif()

    # Имя собранной библиотеки называет CMake: редактор читает его из этого
    # файла, а не составляет сам.
    get_property(multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(multi_config)
        set(record_dir ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>)
    else()
        set(record_dir ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    file(GENERATE OUTPUT ${record_dir}/yqlora_game_module.txt CONTENT "$<TARGET_FILE:${target}>")
endfunction()
