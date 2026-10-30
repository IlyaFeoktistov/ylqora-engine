#include "yqlora/editor/ProjectManager.hpp"

#include "yqlora/editor/ProjectSetup.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/project/ProjectUpgrade.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

namespace yqlora::editor {

ProjectManager::ProjectManager(std::filesystem::path listFile, const SceneSerializer& serializer)
    : m_list(std::move(listFile))
    , m_serializer(serializer)
{
    m_list.load();
    refresh();
}

void ProjectManager::refresh()
{
    m_rows.clear();
    for (const ProjectList::Entry& entry : m_list.entries()) {
        Row row;
        row.directory = entry.directory;
        if (auto info = Project::readInfo(entry.directory)) {
            row.name = std::move(info->name);
            row.engineVersion = std::move(info->engineVersion);
            row.compatibility = info->compatibility;
        } else {
            row.name = toUtf8(entry.directory.filename());
            row.missing = true;
        }
        m_rows.push_back(std::move(row));
    }
}

ProjectManager::OpenResult ProjectManager::open(std::size_t row)
{
    if (row >= m_rows.size() || m_rows[row].missing) {
        return {};
    }
    switch (m_rows[row].compatibility) {
    case FormatCompatibility::Current: break;
    case FormatCompatibility::Older: return {.status = OpenStatus::NeedsUpgrade, .project = std::nullopt};
    case FormatCompatibility::Newer:
    case FormatCompatibility::Unsupported: return {.status = OpenStatus::CannotOpen, .project = std::nullopt};
    }

    auto project = Project::open(m_rows[row].directory, m_serializer.registry());
    if (!project) {
        // Папку убрали, пока менеджер был открыт. Только помечаем строку:
        // список не перестраивается, пока по нему, возможно, идёт цикл.
        m_rows[row].missing = true;

        return {};
    }

    return {.status = OpenStatus::Opened, .project = std::move(project)};
}

std::optional<Project> ProjectManager::create(
    const std::filesystem::path& location,
    const std::string& name,
    std::string& error
)
{
    auto project = createProject(location, name, m_serializer, error);
    if (project) {
        m_list.add(project->directory());
        commit();
    }

    return project;
}

bool ProjectManager::import(const std::filesystem::path& directory, std::string& error)
{
    if (!Project::readInfo(directory)) {
        error = "projects.error.notProject";

        return false;
    }
    m_list.add(directory);
    commit();

    return true;
}

bool ProjectManager::remove(std::size_t row, bool deleteFolder, std::string& error)
{
    if (row >= m_rows.size()) {
        return false;
    }
    const Row& removed = m_rows[row];
    if (deleteFolder && !removed.missing && !deleteProjectFolder(removed.directory, error)) {
        return false;
    }
    m_list.remove(removed.directory);
    commit();

    return true;
}

std::optional<Project> ProjectManager::upgrade(const std::filesystem::path& directory, std::string& error)
{
    if (!upgradeProject(directory, m_serializer)) {
        error = "projects.error.upgradeFailed";

        return std::nullopt;
    }
    refresh();

    return Project::open(directory, m_serializer.registry());
}

void ProjectManager::markOpened(const std::filesystem::path& directory)
{
    m_list.markOpened(directory);
    commit();
}

void ProjectManager::commit()
{
    if (!m_list.save()) {
        log::error("Cannot save the project list");
    }
    refresh();
}

} // namespace yqlora::editor
