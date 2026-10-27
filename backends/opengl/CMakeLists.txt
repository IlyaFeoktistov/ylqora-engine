# Отрисовка ImGui через OpenGL. Отдельной целью, чтобы наши строгие
# предупреждения не применялись к чужому коду.
add_library(imgui_opengl3 STATIC ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_link_libraries(imgui_opengl3 PUBLIC imgui_headers)
target_compile_definitions(imgui_opengl3 PRIVATE "IMGUI_API=${YQLORA_DLL_EXPORT}")

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

# Собирается внутрь библиотеки движка. Заголовок бэкенда виден только фабрике рендереров.
target_include_directories(yqlora_opengl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(yqlora_opengl
    PUBLIC yqlora_engine_code
    PRIVATE glad SDL3::SDL3-static imgui_opengl3
)
yqlora_set_warnings(yqlora_opengl)
