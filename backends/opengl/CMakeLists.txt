add_library(yqlora_opengl STATIC
    OpenGLRenderer.cpp
    GlShader.cpp
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
    PRIVATE glad SDL3::SDL3-static
)
yqlora_set_warnings(yqlora_opengl)
