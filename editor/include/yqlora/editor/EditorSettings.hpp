#pragma once

#include "yqlora/editor/ScriptToolchain.hpp"

#include <filesystem>

namespace yqlora::editor {

// Привычки человека в редакторе: переживают перезапуск, в проект не идут
// (настройки проекта - другое, см. docs/architecture/project-settings.md).
// Файл в папке пользователя, рядом с настройками движка.
class EditorSettings {
public:
    explicit EditorSettings(std::filesystem::path file);

    // Нет файла или он повреждён - остаются значения по умолчанию.
    void load();
    bool save() const;

    // Маленькое окно с картинкой через выбранную камеру в углу окна сцены.
    bool cameraPreview = true;
    // Пути вместо записанных движком в sdk/toolchain.json; в интерфейсе их нет,
    // только в файле.
    ToolchainOverrides toolchain;

private:
    std::filesystem::path m_file;
};

} // namespace yqlora::editor
