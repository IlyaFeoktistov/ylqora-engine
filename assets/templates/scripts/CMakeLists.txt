# Модуль игры проекта. Редактор собирает его сам и передаёт путь к SDK движка
# в yqlora_DIR.
cmake_minimum_required(VERSION 3.26)
project(game LANGUAGES CXX)

find_package(yqlora REQUIRED)

# Новый файл - строкой сюда, новый скрипт - регистрацией в Module.cpp.
yqlora_add_game_module(game
    Module.cpp
    Pulse.cpp
)
