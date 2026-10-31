# Модуль, входная функция которого бросает исключение посреди регистрации.
yqlora_add_game_module(yqlora_throwing_game ThrowingGame.cpp)
set_target_properties(yqlora_throwing_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)
yqlora_set_warnings(yqlora_throwing_game)
