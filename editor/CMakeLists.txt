# Логика редактора без интерфейса: проекты, сессия, выделение, камера, выбор
# объектов. ImGui сюда не подключается, поэтому интерфейс в логику не попадёт.
# Интерфейс (панели, тема) лежит в ui/ и собирается вместе с программой ниже.
add_library(yqlora_editor STATIC
    src/EditorCamera.cpp
    src/Picking.cpp
    src/EditorSession.cpp
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
    main.cpp
    ui/AddNodeDialog.cpp
    ui/EditorApp.cpp
    ui/InspectorPanel.cpp
    ui/ProjectManagerPanel.cpp
    ui/PropertyEditor.cpp
    ui/SceneTreePanel.cpp
    ui/Theme.cpp
    ui/ViewportPanel.cpp
)
target_include_directories(yqlora_app PRIVATE ui)
set_target_properties(yqlora_app PROPERTIES OUTPUT_NAME yqlora)
target_link_libraries(yqlora_app PRIVATE yqlora::yqlora yqlora::editor imgui nlohmann_json::nlohmann_json)
yqlora_set_warnings(yqlora_app)

# Ресурсы движка кладём рядом с исполняемым файлом.
add_custom_command(TARGET yqlora_app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:yqlora_app>/assets
)
