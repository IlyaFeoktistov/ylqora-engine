#include <doctest/doctest.h>

#include <yqlora/editor/CompilerDiagnostics.hpp>

using namespace yqlora;
using namespace yqlora::editor;

TEST_CASE("parseCompilerOutput: GCC and Clang with and without a column")
{
    const auto error = parseCompilerOutput(
        "/home/me/game/scripts/Pulse.cpp:12:5: error: expected ';' before '}' token"
    );
    CHECK(error.level == log::Level::Error);
    CHECK(error.file == "/home/me/game/scripts/Pulse.cpp");
    CHECK(error.line == 12);
    CHECK(error.column == 5);
    CHECK(error.message == "expected ';' before '}' token");

    const auto warning = parseCompilerOutput("Pulse.hpp:7: warning: unused variable 'x' [-Wunused-variable]");
    CHECK(warning.level == log::Level::Warning);
    CHECK(warning.line == 7);
    CHECK(warning.column == 0);

    const auto fatal = parseCompilerOutput("Module.cpp:3:10: fatal error: Pulse.hpp: No such file or directory");
    CHECK(fatal.level == log::Level::Error);
    CHECK(fatal.message == "Pulse.hpp: No such file or directory");

    CHECK(parseCompilerOutput("Pulse.cpp:4:1: note: declared here").level == log::Level::Info);
}

TEST_CASE("parseCompilerOutput: a Windows drive letter is not a location")
{
    const auto error = parseCompilerOutput("C:/Users/me/game/scripts/Pulse.cpp:12:5: error: oops");
    CHECK(error.level == log::Level::Error);
    CHECK(error.file == "C:/Users/me/game/scripts/Pulse.cpp");
    CHECK(error.line == 12);
}

TEST_CASE("parseCompilerOutput: MSVC with a code")
{
    const auto error = parseCompilerOutput(
        "C:\\game\\scripts\\Pulse.cpp(12,5): error C2065: 'x': undeclared identifier"
    );
    CHECK(error.level == log::Level::Error);
    CHECK(error.file == "C:\\game\\scripts\\Pulse.cpp");
    CHECK(error.line == 12);
    CHECK(error.column == 5);
    CHECK(error.message == "'x': undeclared identifier");

    const auto warning = parseCompilerOutput("Pulse.cpp(20): warning C4244: conversion from 'double' to 'float'");
    CHECK(warning.level == log::Level::Warning);
    CHECK(warning.line == 20);
}

TEST_CASE("parseCompilerOutput: linker, ninja and CMake lines without a location")
{
    CHECK(parseCompilerOutput("collect2: error: ld returned 1 exit status").level == log::Level::Error);
    CHECK(parseCompilerOutput("LINK : fatal error LNK1104: cannot open file 'game.dll'").level == log::Level::Error);
    CHECK(parseCompilerOutput("ninja: error: loading 'build.ninja'").level == log::Level::Error);
    CHECK(parseCompilerOutput("CMake Error at CMakeLists.txt:8 (yqlora_add_game_module):").level == log::Level::Error);
    CHECK(parseCompilerOutput("CMake Warning at CMakeLists.txt:8:").level == log::Level::Warning);
}

TEST_CASE("parseCompilerOutput: progress and context lines are information")
{
    const auto progress = parseCompilerOutput("[2/3] Building CXX object CMakeFiles/game.dir/Pulse.cpp.o");
    CHECK(progress.level == log::Level::Info);
    CHECK(progress.message == "[2/3] Building CXX object CMakeFiles/game.dir/Pulse.cpp.o");
    CHECK(parseCompilerOutput("In file included from /game/scripts/Module.cpp:3:").level == log::Level::Info);
    CHECK(parseCompilerOutput("/game/scripts/Pulse.cpp:12:5:   required from here").level == log::Level::Info);
    CHECK(parseCompilerOutput("").level == log::Level::Info);
}
