#pragma once

#include "yqlora/core/Log.hpp"

#include <string>
#include <string_view>

namespace yqlora::editor {

// Строка вывода сборки, разобранная по правилам GCC, Clang, MSVC и CMake.
struct CompilerDiagnostic {
    // Ошибка и предупреждение компилятора, компоновщика или CMake; прочее
    // (ход сборки, note:, "In file included from") - Info.
    log::Level level = log::Level::Info;
    // Место, если строка его называет: путь как напечатан, строка и столбец с 1;
    // 0 - не назван.
    std::string file;
    int line = 0;
    int column = 0;
    // Текст после места и вида ("expected ';' before '}'"); без места - строка целиком.
    std::string message;
};

// Разбирает одну строку вывода. Любая строка даёт запись: незнакомая - Info с
// текстом целиком.
CompilerDiagnostic parseCompilerOutput(std::string_view line);

} // namespace yqlora::editor
