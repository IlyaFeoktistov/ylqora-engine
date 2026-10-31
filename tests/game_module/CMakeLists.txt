# Модуль игры с тестовым скриптом. Модуль на папку один: в ней он пишет
# запись пути к библиотеке (см. yqlora_add_game_module).
yqlora_add_game_module(yqlora_test_game TestGame.cpp)
set_target_properties(yqlora_test_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
yqlora_set_warnings(yqlora_test_game)

add_subdirectory(throwing)
