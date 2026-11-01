#pragma once

#include "yqlora/reflection/Property.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {
class ResourceCache;
class ResourceDatabase;
} // namespace yqlora

namespace yqlora::editor {

// Что можно выбрать в поле ссылки на ресурс (окно выбора, как в Unity).
struct ResourceChoice {
    ResourceId id = 0;
    // Имя в списке: имя встроенного меша, путь файла или имя меша в модели.
    std::string label;
    // Файл модели, в которой лежит меш; пусто у встроенных и у файлов.
    std::string group;
    // Встроенный ресурс движка (cube, sphere, plane), а не файл проекта.
    bool builtin = false;
};

// Вид ресурса, который ждёт поле с подсказкой hint: MeshData::kResourceKind,
// TextureData::kResourceKind или kSceneKind. Пусто - поле не ссылка на ресурс.
inline constexpr std::string_view kSceneKind = "scene";
std::string_view fieldKind(PropertyHint hint);

// Всё, что подходит полю: встроенные ресурсы, затем ресурсы проекта по
// путям (у мешей - по файлам моделей). Модели загружаются: ID их мешей
// появляются при загрузке. resources - nullptr без проекта.
std::vector<ResourceChoice> resourceChoices(PropertyHint hint, const ResourceDatabase* resources, ResourceCache& cache);

// Что положить в поле, если на него отпустили ресурс id: сам ресурс, если
// он подходит, а модель на поле меша - её первый меш, как в Unity. nullopt -
// ресурс не подходит.
std::optional<ResourceId> resourceForField(
    PropertyHint hint,
    ResourceId id,
    const ResourceDatabase* resources,
    ResourceCache& cache
);

} // namespace yqlora::editor
