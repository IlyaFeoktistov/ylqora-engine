#include "yqlora/editor/ModuleCopies.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/project/Project.hpp"

#include <format>

namespace yqlora::editor {

namespace {

namespace fs = std::filesystem;

constexpr const char* kReloadFolder = "reload";
constexpr const char* kCopyPrefix = "game_";

} // namespace

ModuleCopies::ModuleCopies(const fs::path& projectDirectory)
    : m_directory(projectDirectory / Project::buildDirectoryName() / kReloadFolder)
{
}

std::optional<fs::path> ModuleCopies::copy(const fs::path& library)
{
    std::error_code ec;
    fs::create_directories(m_directory, ec);
    // Номер, который ещё не занят: копия прошлого запуска могла остаться заблокированной.
    fs::path target;
    do {
        target = m_directory / fromUtf8(std::format("{}{}{}", kCopyPrefix, m_next++, toUtf8(library.extension())));
    } while (fs::exists(target, ec));
    if (!fs::copy_file(library, target, ec)) {
        log::error("Cannot copy the game module to '{}': {}", toUtf8(target), ec.message());

        return std::nullopt;
    }

    return target;
}

void ModuleCopies::removeUnused(const fs::path& inUse) const
{
    std::error_code ec;
    for (const fs::directory_entry& entry : fs::directory_iterator(m_directory, ec)) {
        if (entry.path() != inUse) {
            fs::remove(entry.path(), ec);
        }
    }
}

} // namespace yqlora::editor
