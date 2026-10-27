#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/scene/Node.hpp"

#include <filesystem>
#include <string>
#include <string_view>

namespace yqlora {

class Scene;
class TypeRegistry;

// Сохраняет сцену в JSON и загружает обратно. Узлы и компоненты создаются
// по именам типов из реестра, значения читаются и пишутся через их свойства.
class YQLORA_API SceneSerializer {
public:
    // Расширение файлов сцен.
    static constexpr std::string_view fileExtension()
    {
        return ".yscene";
    }

    explicit SceneSerializer(const TypeRegistry& registry);

    std::string toJson(const Scene& scene) const;

    // При ошибке возвращает false и оставляет сцену как была.
    bool fromJson(Scene& scene, std::string_view json) const;

    bool save(const Scene& scene, const std::filesystem::path& file) const;
    bool load(Scene& scene, const std::filesystem::path& file) const;

private:
    struct Format;

    // ID узла из файла: Node::setId закрыт для всех, кроме сериализатора.
    static void assignId(Node& node, NodeId id);

    const TypeRegistry& m_registry;
};

} // namespace yqlora
