#include "yqlora/project/ProjectUpgrade.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <memory>
#include <utility>
#include <vector>

namespace yqlora {

bool upgradeProject(const std::filesystem::path& directory, const SceneSerializer& serializer)
{
    const auto info = Project::readInfo(directory);
    if (!info
        || (info->compatibility != FormatCompatibility::Older && info->compatibility != FormatCompatibility::Current)) {
        return false;
    }

    // Чтение переводит сцену на текущий формат, запись сохраняет уже в нём.
    // Сцену нельзя переместить в памяти (на неё указывают узлы), поэтому по указателю.
    std::vector<std::pair<std::filesystem::path, std::unique_ptr<Scene>>> scenes;
    std::error_code ec;
    for (auto it = std::filesystem::recursive_directory_iterator(directory, ec);
         !ec && it != std::filesystem::recursive_directory_iterator();
         it.increment(ec)) {
        if (it->is_regular_file(ec) && it->path().extension() == SceneSerializer::fileExtension()) {
            Scene& scene = *scenes.emplace_back(it->path(), std::make_unique<Scene>(EngineServices::none())).second;
            if (!serializer.load(scene, it->path())) {
                log::error("Project upgrade: scene '{}' is not read, nothing is changed", toUtf8(it->path()));

                return false;
            }
        }
    }
    if (!ProjectSettings::upgradeFiles(directory)) {
        return false;
    }
    for (const auto& [path, scene] : scenes) {
        if (!serializer.save(*scene, path)) {
            log::error("Project upgrade: cannot write scene '{}'", toUtf8(path));

            return false;
        }
    }

    // Файл проекта последним: пока он старый, проект снова предложат перевести,
    // а повторный перевод уже переведённых файлов ничего не меняет.
    return Project::upgradeFile(directory);
}

} // namespace yqlora
