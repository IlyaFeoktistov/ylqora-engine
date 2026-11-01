#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace yqlora {

// Что загрузчику нужно знать о файле. Позже здесь же настройки импорта из .meta.
struct LoadRequest {
    // Полный путь к файлу.
    std::filesystem::path file;
};

// Один ресурс из файла. Пустой ключ - ресурс самого файла (ID из его .meta),
// остальные - вложенные ресурсы (меши внутри модели).
struct LoadedResource {
    std::string key;
    // Вид данных: T::kResourceKind у типа данных (MeshData и другие).
    std::string_view kind;
    std::shared_ptr<const void> data;
};

// Ресурсы файла или текст ошибки, если файл не прочитался.
struct LoadResult {
    std::vector<LoadedResource> resources;
    std::string error;
};

template <typename T>
LoadedResource makeLoadedResource(std::string key, T data)
{
    return {std::move(key), T::kResourceKind, std::make_shared<const T>(std::move(data))};
}

inline LoadResult loadError(std::string message)
{
    return {{}, std::move(message)};
}

// Читает файлы одного вида (glTF, картинки) в данные на стороне CPU. Новый
// вид ресурса - новый загрузчик, кэш и остальной движок не меняются.
// Загрузчик не бросает исключений: ошибка возвращается в LoadResult::error.
class YQLORA_API ResourceLoader {
public:
    virtual ~ResourceLoader() = default;

    // Расширения файлов в нижнем регистре, с точкой: ".glb".
    virtual std::span<const std::string_view> extensions() const = 0;

    virtual LoadResult load(const LoadRequest& request) const = 0;
};

} // namespace yqlora
