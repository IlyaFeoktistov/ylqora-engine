#pragma once

namespace yqlora {
struct TextureData;
} // namespace yqlora

namespace yqlora::opengl {

// Текстура в видеопамяти с фильтром, повтором и мипмапами из настроек импорта.
class GlTexture {
public:
    // Для интерфейса цвета берутся как есть, без перевода из sRGB: ImGui
    // рисует в обычный, не sRGB, буфер, и переведённая картинка была бы темнее.
    enum class Use {
        Scene,
        Interface,
    };

    GlTexture(const TextureData& data, Use use);
    ~GlTexture();

    GlTexture(const GlTexture&) = delete;
    GlTexture& operator=(const GlTexture&) = delete;

    unsigned int id() const
    {
        return m_texture;
    }

private:
    unsigned int m_texture = 0;
};

} // namespace yqlora::opengl
