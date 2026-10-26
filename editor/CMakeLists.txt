# Редактор: камера, выбор объектов, позже интерфейс. Игре не нужен.
add_library(yqlora_editor STATIC
    src/EditorCamera.cpp
    src/Picking.cpp
    src/Project.cpp
    src/ProjectList.cpp
)
add_library(yqlora::editor ALIAS yqlora_editor)

target_include_directories(yqlora_editor PUBLIC include)
target_link_libraries(yqlora_editor
    PUBLIC yqlora::internal
    PRIVATE nlohmann_json::nlohmann_json
)
yqlora_set_warnings(yqlora_editor)

# Программа yqlora: движок с редактором. Имя цели другое, потому что
# yqlora уже занято библиотекой движка.
add_executable(yqlora_app
    app/main.cpp
    app/EditorApp.cpp
    app/ProjectManagerPanel.cpp
    app/SceneTreePanel.cpp
    app/ViewportPanel.cpp
)
set_target_properties(yqlora_app PROPERTIES OUTPUT_NAME yqlora)
target_link_libraries(yqlora_app PRIVATE yqlora::yqlora yqlora::editor imgui)
yqlora_set_warnings(yqlora_app)

# Ресурсы движка кладём рядом с исполняемым файлом.
add_custom_command(TARGET yqlora_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:yqlora_app>/assets
)
