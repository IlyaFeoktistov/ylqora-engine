#include "yqlora/editor/ProjectFiles.hpp"

#include "yqlora/core/EngineServices.hpp"
#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/editor/ProjectSetup.hpp"
#include "yqlora/project/Project.hpp"
#include "yqlora/resource/ResourceDatabase.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <algorithm>

namespace yqlora::editor {

namespace {

namespace fs = std::filesystem;

constexpr const char* kNewSceneName = "New Scene";
constexpr const char* kNewFolderName = "New Folder";
// "New Scene 2", "New Scene 3"...: первая копия имени получает номер 2.
constexpr int kFirstDuplicateNumber = 2;

void scanDirectory(const fs::path& root, const fs::path& relative, ProjectFiles::Entry& out)
{
    std::error_code ec;
    for (const fs::directory_entry& item : fs::directory_iterator(root / relative, ec)) {
        const std::string name = toUtf8(item.path().filename());
        if (name.empty() || name.front() == '.' || item.path().extension() == ResourceDatabase::metaExtension()
            || (relative.empty()
                && std::ranges::find(Project::reservedNames(), name) != Project::reservedNames().end())) {
            continue;
        }
        ProjectFiles::Entry entry;
        entry.name = name;
        entry.path = relative / item.path().filename();
        entry.directory = item.is_directory(ec);
        if (entry.directory) {
            scanDirectory(root, entry.path, entry);
        }
        out.children.push_back(std::move(entry));
    }
    std::ranges::sort(out.children, [](const ProjectFiles::Entry& a, const ProjectFiles::Entry& b) {
        if (a.directory != b.directory) {
            return a.directory;
        }

        return a.name < b.name;
    });
}

// Первое свободное имя: "base.ext", "base 2.ext", "base 3.ext"...
fs::path freeName(const fs::path& directory, const std::string& base, const std::string& extension)
{
    std::error_code ec;
    fs::path candidate = directory / fromUtf8(base + extension);
    for (int i = kFirstDuplicateNumber; fs::exists(candidate, ec); ++i) {
        candidate = directory / fromUtf8(base + " " + std::to_string(i) + extension);
    }

    return candidate;
}

} // namespace

ProjectFiles::ProjectFiles(Project& project)
    : m_project(project)
{
}

ProjectFiles::Entry ProjectFiles::scan() const
{
    Entry root;
    root.name = m_project.name();
    root.directory = true;
    scanDirectory(m_project.directory(), {}, root);

    return root;
}

bool ProjectFiles::isScene(const fs::path& path)
{
    return path.extension() == SceneSerializer::fileExtension();
}

bool ProjectFiles::canModify(const fs::path& path) const
{
    const fs::path normal = path.lexically_normal();
    if (normal.empty() || normal == "." || normal.is_absolute()) {
        return false;
    }
    // ".." в начале - путь выходит из папки проекта. Файл проекта и
    // настройки правит не панель файлов.
    const auto first = *normal.begin();
    const bool reserved = std::ranges::find(Project::reservedNames(), toUtf8(first)) != Project::reservedNames().end();

    return first != ".." && !reserved && !m_project.directory().empty();
}

std::optional<fs::path> ProjectFiles::createScene(const fs::path& folder, const SceneSerializer& serializer)
{
    if (!folder.empty() && !canModify(folder)) {
        return std::nullopt;
    }
    const fs::path file = freeName(
        m_project.directory() / folder,
        kNewSceneName,
        std::string(SceneSerializer::fileExtension())
    );
    if (!serializer.save(Scene(EngineServices::none()), file)) {
        return std::nullopt;
    }
    const fs::path relative = fs::relative(file, m_project.directory());
    m_project.resources().add(relative);

    return relative;
}

std::optional<fs::path> ProjectFiles::createFolder(const fs::path& folder)
{
    if (!folder.empty() && !canModify(folder)) {
        return std::nullopt;
    }
    const fs::path directory = freeName(m_project.directory() / folder, kNewFolderName, "");
    std::error_code ec;
    if (!fs::create_directory(directory, ec)) {
        return std::nullopt;
    }

    return fs::relative(directory, m_project.directory());
}

std::optional<fs::path> ProjectFiles::rename(const fs::path& path, const std::string& newName)
{
    if (!canModify(path) || !isValidFileName(newName) || newName == Project::fileName()) {
        return std::nullopt;
    }
    const fs::path target = path.parent_path() / fromUtf8(newName);
    if (target.extension() == ResourceDatabase::metaExtension()) {
        return std::nullopt;
    }
    std::error_code ec;
    if (fs::exists(m_project.directory() / target, ec)) {
        return std::nullopt;
    }
    fs::rename(m_project.directory() / path, m_project.directory() / target, ec);
    if (ec) {
        log::error("Cannot rename '{}': {}", toUtf8(path), ec.message());

        return std::nullopt;
    }
    m_project.resources().moved(path, target);

    return target;
}

bool ProjectFiles::remove(const fs::path& path)
{
    if (!canModify(path)) {
        return false;
    }
    std::error_code ec;
    fs::remove_all(m_project.directory() / path, ec);
    if (ec) {
        log::error("Cannot delete '{}': {}", toUtf8(path), ec.message());

        return false;
    }
    m_project.resources().removed(path);

    return true;
}

} // namespace yqlora::editor
