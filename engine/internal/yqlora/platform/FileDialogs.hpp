#pragma once

#include "yqlora/core/Export.hpp"

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>

namespace yqlora {

class Window;

// Системные диалоги выбора файлов и папок поверх окна.
class YQLORA_API FileDialogs {
public:
    explicit FileDialogs(Window& window);
    ~FileDialogs();

    FileDialogs(const FileDialogs&) = delete;
    FileDialogs& operator=(const FileDialogs&) = delete;

    // Диалог выбора папки. Ответ придёт в callback из deliver(), в главном
    // потоке; nullopt, если выбор отменили или диалог недоступен.
    // Пока открыт один диалог, второй не открывается.
    using FolderCallback = std::function<void(std::optional<std::filesystem::path>)>;
    void showFolderDialog(const std::filesystem::path& start, FolderCallback callback);

    // Отдаёт готовый ответ диалога. Вызывается раз в кадр.
    void deliver();

private:
    // Диалог SDL может ответить из другого потока: ответ кладётся сюда,
    // а отдаётся в deliver().
    struct FolderDialog;

    Window& m_window;
    std::unique_ptr<FolderDialog> m_folderDialog;
};

} // namespace yqlora
