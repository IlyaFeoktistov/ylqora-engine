# Модуль игры со скриптами-примерами. Файл называется как библиотека игры
# проекта (game.dll, libgame.so): его можно положить в .build/ проекта как есть.
yqlora_add_game_module(yqlora_example_scripts Rotator.cpp)
set_target_properties(yqlora_example_scripts PROPERTIES
    # Отдельно от bin/: рядом с редактором библиотеке игры делать нечего.
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/scripts
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/scripts
)
yqlora_set_warnings(yqlora_example_scripts)
