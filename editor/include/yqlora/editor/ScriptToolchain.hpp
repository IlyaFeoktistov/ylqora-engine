#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace yqlora::editor {

// Пути, которыми человек заменяет записанные движком (другая машина, другой
// путь к компилятору). Пустой - взять из записи. Хранятся в настройках
// редактора: зависят от машины и в git не идут.
struct ToolchainOverrides {
    std::filesystem::path cmake;
    std::filesystem::path makeProgram;
    std::filesystem::path cCompiler;
    std::filesystem::path cxxCompiler;
};

// Набор инструментов, которым собирается модуль игры: тот же, что у движка
// (sdk/toolchain.json), чтобы скрипт был собран под ABI библиотеки, в которую
// загружается.
struct ScriptToolchain {
    std::filesystem::path cmake;
    std::string generator;
    std::filesystem::path makeProgram;
    std::filesystem::path cCompiler;
    std::filesystem::path cxxCompiler;
    std::string compilerId;
    std::string compilerVersion;
    std::string architecture;
    // Только для MSVC: поднимает окружение Visual Studio перед CMake.
    std::filesystem::path vcvarsall;

    // Меняется вместе с любым полем: сменившийся набор получает новую папку
    // сборки, а не старый CMakeCache.txt.
    std::string fingerprint() const;
};

// SDK движка: рядом с программой редактора.
std::filesystem::path sdkDirectory();

// Читает sdk/toolchain.json. Нет файла или он повреждён - nullopt.
std::optional<ScriptToolchain> loadScriptToolchain(const std::filesystem::path& sdk);

void applyOverrides(ScriptToolchain& toolchain, const ToolchainOverrides& overrides);

} // namespace yqlora::editor
