#pragma once

#include "yqlora/math/Math.hpp"
#include "yqlora/scene/Node3D.hpp"

#include <string>

namespace yqlora {

// Рисует меш в положении узла.
class MeshInstance3D : public Node3D {
public:
    explicit MeshInstance3D(std::string name = "MeshInstance3D");

    std::string_view typeName() const override
    {
        return "MeshInstance3D";
    }

    // Пока только встроенные меши: "cube", "sphere", "plane". Позже здесь же
    // будет путь к файлу модели. С неизвестным именем узел ничего не рисует.
    const std::string& mesh() const
    {
        return m_mesh;
    }

    void setMesh(std::string mesh)
    {
        m_mesh = std::move(mesh);
    }

    // Цвет поверхности, RGBA от 0 до 1.
    const Vec4& color() const
    {
        return m_color;
    }

    void setColor(const Vec4& color)
    {
        m_color = color;
    }

private:
    std::string m_mesh = "cube";
    Vec4 m_color{0.8f, 0.8f, 0.8f, 1.0f};
};

} // namespace yqlora
