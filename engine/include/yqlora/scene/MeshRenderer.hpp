#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/resource/BuiltinResources.hpp"
#include "yqlora/scene/Component.hpp"

namespace yqlora {

// Рисует меш в положении узла. Работает на Node3D: положение берётся из его Transform.
class YQLORA_API MeshRenderer final : public Component {
public:
    YQLORA_TYPE(MeshRenderer)

    // Ресурс меша. Пока работают только встроенные (см. builtin::kCubeMesh),
    // с неизвестным мешем компонент ничего не рисует.
    ResourceRef mesh() const
    {
        return m_mesh;
    }

    void setMesh(ResourceRef mesh)
    {
        m_mesh = mesh;
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
    ResourceRef m_mesh{builtin::kCubeMesh};
    Vec4 m_color{0.8f, 0.8f, 0.8f, 1.0f};
};

} // namespace yqlora
