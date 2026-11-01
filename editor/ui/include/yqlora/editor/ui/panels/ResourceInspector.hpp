#pragma once

#include "yqlora/editor/ResourceInspection.hpp"

#include <optional>

namespace yqlora::editor {

struct EditorContext;

// Инспектор выбранного в "Файлах" ресурса: что в файле (меши модели,
// размер и превью текстуры) и настройки импорта. Правка настроек копится
// здесь и попадает в .meta по кнопке "Применить", как в Unity.
class ResourceInspector {
public:
    void draw(EditorContext& context, const ResourceInspection& inspection);

private:
    void drawModel(const ResourceInspection& inspection);
    void drawTexture(EditorContext& context, const ResourceInspection& inspection);
    void drawImportSettings(EditorContext& context, const ResourceInspection& inspection);

    // Чьи настройки сейчас правятся: другой файл - правка начинается заново.
    ResourceId m_id = 0;
    std::optional<ModelImport> m_model;
    std::optional<TextureSampling> m_texture;
};

} // namespace yqlora::editor
