# Модуль игры со скриптами-примерами. Файл называется как библиотека игры
# проекта (game.dll, libgame.so): его можно положить в .build/ проекта как есть.
add_library(yqlora_example_scripts SHARED Rotator.cpp)
target_link_libraries(yqlora_example_scripts PRIVATE yqlora::yqlora)
set_target_properties(yqlora_example_scripts PROPERTIES
    OUTPUT_NAME game
    # Отдельно от bin/: рядом с редактором библиотеке игры делать нечего.
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/scripts
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/scripts
)
# Имя должно совпасть с platform::sharedLibraryFileName("game"): MinGW по
# умолчанию дал бы libgame.dll.
if(WIN32)
    set_target_properties(yqlora_example_scripts PROPERTIES PREFIX "")
endif()
yqlora_set_warnings(yqlora_example_scripts)
