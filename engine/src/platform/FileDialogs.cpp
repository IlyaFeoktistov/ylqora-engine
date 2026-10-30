#include "yqlora/platform/FileDialogs.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/platform/Window.hpp"

#include <SDL3/SDL.h>

#include <mutex>
#include <string>

namespace yqlora {

struct FileDialogs::FolderDialog {
    FolderCallback callback;
    std::mutex mutex;
    bool answered = false;
    std::optional<std::filesystem::path> folder;
};

FileDialogs::FileDialogs(Window& window)
    : m_window(window)
{
}

FileDialogs::~FileDialogs()
{
    // Диалог мог остаться открытым: SDL ещё может вызвать его callback.
    // Тогда отдаём состояние ему насовсем, чтобы указатель не повис.
    if (m_folderDialog) {
        const std::lock_guard lock(m_folderDialog->mutex);
        if (!m_folderDialog->answered) {
            static_cast<void>(m_folderDialog.release());
        }
    }
}

void FileDialogs::showFolderDialog(const std::filesystem::path& start, FolderCallback callback)
{
    if (m_folderDialog) {
        return;
    }
    m_folderDialog = std::make_unique<FolderDialog>();
    m_folderDialog->callback = std::move(callback);

    const auto onAnswer = [](void* userdata, const char* const* files, int /*filter*/) {
        auto* dialog = static_cast<FolderDialog*>(userdata);
        std::optional<std::filesystem::path> folder;
        if (!files) {
            log::warn("Folder dialog is not available: {}", SDL_GetError());
        } else if (files[0]) {
            folder = fromUtf8(files[0]);
        }
        const std::lock_guard lock(dialog->mutex);
        dialog->folder = std::move(folder);
        dialog->answered = true;
    };
    const std::string startUtf8 = toUtf8(start);
    SDL_ShowOpenFolderDialog(onAnswer, m_folderDialog.get(), m_window.sdlWindow(), startUtf8.c_str(), false);
}

void FileDialogs::deliver()
{
    if (!m_folderDialog) {
        return;
    }
    {
        const std::lock_guard lock(m_folderDialog->mutex);
        if (!m_folderDialog->answered) {
            return;
        }
    }
    // Callback может открыть новый диалог, поэтому сначала освобождаем место.
    const std::unique_ptr<FolderDialog> dialog = std::move(m_folderDialog);
    dialog->callback(std::move(dialog->folder));
}

} // namespace yqlora
