# Редактор: камера, выбор объектов, позже интерфейс. Игре не нужен.
add_library(yqlora_editor STATIC
    src/EditorCamera.cpp
    src/Picking.cpp
)
add_library(yqlora::editor ALIAS yqlora_editor)

target_include_directories(yqlora_editor PUBLIC include)
target_link_libraries(yqlora_editor PUBLIC yqlora::internal)
yqlora_set_warnings(yqlora_editor)
