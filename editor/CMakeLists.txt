# Логика редактора без интерфейса: проекты, сессия, история правок, выделение,
# камера, выбор объектов. ImGui сюда не подключается, поэтому интерфейс в логику не попадёт.
add_library(yqlora_editor STATIC
    src/EditorCamera.cpp
    src/Picking.cpp
    src/EditorSession.cpp
    src/Project.cpp
    src/ProjectFiles.cpp
    src/ProjectList.cpp
    src/SceneCommands.cpp
    src/TransformEdit.cpp
    src/EditHistory.cpp
)
add_library(yqlora::editor ALIAS yqlora_editor)

target_include_directories(yqlora_editor PUBLIC include)
target_link_libraries(yqlora_editor
    PUBLIC yqlora::internal
    PRIVATE nlohmann_json::nlohmann_json
)
yqlora_set_warnings(yqlora_editor)

# Интерфейс редактора на ImGui: панели, диалоги, гизмо, тема. Только вызывает
# логику выше. Программа с main лежит в apps/editor.
add_library(yqlora_editor_ui STATIC
    ui/EditorApp.cpp
    ui/core/Actions.cpp
    ui/dialogs/AddNodeDialog.cpp
    ui/dialogs/UnsavedChangesDialog.cpp
    ui/gizmos/AxisGizmo.cpp
    ui/gizmos/CameraGizmo.cpp
    ui/gizmos/GizmoLayer.cpp
    ui/gizmos/GridGizmo.cpp
    ui/gizmos/ManipulatorGizmo.cpp
    ui/gizmos/SelectionGizmo.cpp
    ui/panels/FileSystemPanel.cpp
    ui/panels/InspectorPanel.cpp
    ui/panels/ProjectManagerPanel.cpp
    ui/panels/SceneTreePanel.cpp
    ui/panels/ViewportPanel.cpp
    ui/style/Theme.cpp
    ui/widgets/PropertyEditor.cpp
)
add_library(yqlora::editor_ui ALIAS yqlora_editor_ui)

target_include_directories(yqlora_editor_ui PUBLIC ui)
target_link_libraries(yqlora_editor_ui
    PUBLIC yqlora::yqlora yqlora::editor imgui
    PRIVATE imguizmo nlohmann_json::nlohmann_json
)
yqlora_set_warnings(yqlora_editor_ui)
