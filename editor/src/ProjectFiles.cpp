#include "yqlora/editor/ProjectFiles.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/editor/Project.hpp"
#include "yqlora/scene/Scene.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <algorithm>

namespace yqlora::projectfiles {

namespace {

namespace fs = std::filesystem;

std::string toUtf8(const fs::path& path)
{
    const std::u8string text = path.u8string();

    return {text.begin(), text.end()};
}

fs::path fromUtf8(const std::string& text)
{
    return fs::path(std::u8string(text.begin(), text.end()));
}

void scanDirectory(const fs::path& root, const fs::path& relative, Entry& out)
{
    std::error_code ec;
    for (const fs::directory_entry& item : fs::directory_iterator(root / relative, ec)) {
        const std::string name = toUtf8(item.path().filename());
        if (name.empty() || name.front() == '.' || (relative.empty() && name == Project::kFileName)) {
            continue;
        }
        Entry entry;
        entry.name = name;
        entry.path = relative / item.path().filename();
        entry.directory = item.is_directory(ec);
        if (entry.directory) {
            scanDirectory(root, entry.path, entry);
        }
        out.children.push_back(std::move(entry));
    }
    std::ranges::sort(out.children, [](const Entry& a, const Entry& b) {
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
    for (int i = 2; fs::exists(candidate, ec); ++i) {
        candidate = directory / fromUtf8(base + " " + std::to_string(i) + extension);
    }

    return candidate;
}

} // namespace

Entry scan(const Project& project)
{
    Entry root;
    root.name = project.name;
    root.directory = true;
    scanDirectory(project.directory, {}, root);

    return root;
}

bool isScene(const fs::path& path)
{
    return path.extension() == kSceneExtension;
}

bool canModify(const Project& project, const fs::path& path)
{
    const fs::path normal = path.lexically_normal();
    if (normal.empty() || normal == "." || normal.is_absolute() || normal == Project::kFileName) {
        return false;
    }
    // ".." в начале - путь выходит из папки проекта.
    const auto first = *normal.begin();

    return first != ".." && !project.directory.empty();
}

std::optional<fs::path> createScene(const Project& project, const fs::path& folder, const SceneSerializer& serializer)
{
    if (!folder.empty() && !canModify(project, folder)) {
        return std::nullopt;
    }
    const fs::path file = freeName(project.directory / folder, "New Scene", kSceneExtension);
    if (!serializer.save(Scene{}, file)) {
        return std::nullopt;
    }

    return fs::relative(file, project.directory);
}

std::optional<fs::path> createFolder(const Project& project, const fs::path& folder)
{
    if (!folder.empty() && !canModify(project, folder)) {
        return std::nullopt;
    }
    const fs::path directory = freeName(project.directory / folder, "New Folder", "");
    std::error_code ec;
    if (!fs::create_directory(directory, ec)) {
        return std::nullopt;
    }

    return fs::relative(directory, project.directory);
}

std::optional<fs::path> rename(const Project& project, const fs::path& path, const std::string& newName)
{
    if (!canModify(project, path) || !Project::isValidName(newName) || newName == Project::kFileName) {
        return std::nullopt;
    }
    const fs::path target = path.parent_path() / fromUtf8(newName);
    std::error_code ec;
    if (fs::exists(project.directory / target, ec)) {
        return std::nullopt;
    }
    fs::rename(project.directory / path, project.directory / target, ec);
    if (ec) {
        log::error("Cannot rename '{}': {}", toUtf8(path), ec.message());

        return std::nullopt;
    }

    return target;
}

bool remove(const Project& project, const fs::path& path)
{
    if (!canModify(project, path)) {
        return false;
    }
    std::error_code ec;
    fs::remove_all(project.directory / path, ec);
    if (ec) {
        log::error("Cannot delete '{}': {}", toUtf8(path), ec.message());

        return false;
    }

    return true;
}

} // namespace yqlora::projectfiles
