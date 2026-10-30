#pragma once

#include "yqlora/opengl/GlShader.hpp"

#include "yqlora/render/FrameData.hpp"

namespace yqlora::opengl {

// Бесконечная сетка редактора на плоскости Y = 0. Рисуется квадрат под камерой,
// который едет вместе с ней, а линии считаются во фрагментном шейдере
// по мировым координатам. С расстоянием сетка гаснет, поэтому края квадрата не видно.
class GridPass {
public:
    GridPass();
    ~GridPass();

    GridPass(const GridPass&) = delete;
    GridPass& operator=(const GridPass&) = delete;

    // Рисует поверх уже нарисованной сцены: глубину читает, но не пишет.
    void draw(const CameraData& camera) const;

private:
    GlShader m_shader;
    unsigned int m_vertexArray = 0;
    unsigned int m_vertexBuffer = 0;
    int m_viewProjectionLocation = -1;
    int m_cameraPositionLocation = -1;
    int m_cellSizeLocation = -1;
    int m_fadeDistanceLocation = -1;
};

} // namespace yqlora::opengl
