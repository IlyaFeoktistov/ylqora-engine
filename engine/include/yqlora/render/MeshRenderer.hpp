#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/math/Math.hpp"
#include "yqlora/reflection/Property.hpp"
#include "yqlora/scene/Component.hpp"

namespace yqlora {

// Рисует меш в положении узла. Живёт только на Node3D (requiresNode): положение берётся из его Transform.
class YQLORA_API MeshRenderer final : public Component {
public:
    YQLORA_TYPE(MeshRenderer, Component)

    // Ресурс меша: встроенный (builtin::kCubeMesh и другие) или из файла
    // проекта. Пустая ссылка (по умолчанию) - меша нет, и компонент ничего
    // не рисует, как и с отсутствующим или битым мешем.
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
    ResourceRef m_mesh;
    Vec4 m_color{0.8f, 0.8f, 0.8f, 1.0f};
};

} // namespace yqlora
