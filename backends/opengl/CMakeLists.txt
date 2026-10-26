# Отрисовка ImGui через OpenGL. Отдельной целью, чтобы наши строгие
# предупреждения не применялись к чужому коду.
add_library(imgui_opengl3 STATIC ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(imgui_opengl3 PUBLIC imgui)

add_library(yqlora_opengl STATIC
    OpenGLRenderer.cpp
    GlShader.cpp
    GlRenderTarget.cpp
    GridPass.cpp
    GlMesh.cpp
    MeshPass.cpp
    OutlinePass.cpp
)
add_library(yqlora::opengl ALIAS yqlora_opengl)

# Заголовок бэкенда виден только фабрике рендереров, но не играм.
target_include_directories(yqlora_opengl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(yqlora_opengl
    PUBLIC yqlora::internal
    PRIVATE glad SDL3::SDL3-static imgui_opengl3
)
yqlora_set_warnings(yqlora_opengl)
