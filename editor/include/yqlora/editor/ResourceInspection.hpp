#pragma once

#include "yqlora/render/TextureData.hpp"
#include "yqlora/resource/ResourceCache.hpp"
#include "yqlora/resource/ResourceId.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {
class Project;
} // namespace yqlora

namespace yqlora::editor {

class EditorSession;

struct MeshSummary {
    ResourceId id = 0;
    // Ключ без вида: "Door", "Door#1".
    std::string label;
    std::size_t vertices = 0;
    std::size_t triangles = 0;
    std::size_t submeshes = 0;
};

// Настройки импорта, которые редактор показывает и меняет. Свои у каждого
// вида; у файла другого вида настроек нет.
struct ModelImport {
    float scale = 1.0f;

    bool operator==(const ModelImport&) const = default;
};

// Что инспектор показывает о выбранном файле ресурса.
struct ResourceInspection {
    ResourceId id = 0;
    std::filesystem::path path;
    // Вид по загрузчику; пусто - движок такие файлы не читает (сцена и др.).
    std::string_view kind;
    // nullopt - файл не читается загрузчиком.
    std::optional<ResourceStatus> status;
    // Модель: её меши и число узлов.
    std::vector<MeshSummary> meshes;
    std::size_t nodes = 0;
    // Текстура: данные для превью и размер.
    LoadedData<TextureData> texture;
    // Настройки импорта, как они записаны в .meta.
    std::optional<ModelImport> modelImport;
    std::optional<TextureSampling> textureImport;
};

// nullopt, если проекта нет или ресурса id в нём нет. Файл загружается,
// если ещё не был.
std::optional<ResourceInspection> inspectResource(EditorSession& session, ResourceId id);

// Записывает настройки импорта в .meta и перечитывает файл: новая ревизия,
// новые данные везде, где ресурс используется. Истории правок это не
// касается, как и в Unity: файл и .meta - не сцена. false, если .meta не
// записалась.
bool applyImportSettings(EditorSession& session, ResourceId id, const ModelImport& settings);
bool applyImportSettings(EditorSession& session, ResourceId id, const TextureSampling& settings);

} // namespace yqlora::editor
