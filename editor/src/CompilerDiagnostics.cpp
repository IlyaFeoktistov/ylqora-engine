#include "yqlora/editor/CompilerDiagnostics.hpp"

#include <array>
#include <cctype>
#include <optional>
#include <utility>

namespace yqlora::editor {

namespace {

struct Kind {
    std::string_view word;
    log::Level level;
};

// Слова вида после места. "fatal error" раньше "error", иначе совпало бы короткое.
constexpr std::array<Kind, 4> kKinds{{
    {"fatal error", log::Level::Error},
    {"error", log::Level::Error},
    {"warning", log::Level::Warning},
    {"note", log::Level::Info},
}};

constexpr int kDecimalBase = 10;
// Длина пути "C:" в начале: двоеточие диска - не разделитель места.
constexpr std::size_t kDriveLength = 2;

bool isDigit(char c)
{
    return std::isdigit(static_cast<unsigned char>(c)) != 0;
}

// Число с позиции pos; pos сдвигается за него. nullopt, если цифр нет.
std::optional<int> readNumber(std::string_view text, std::size_t& pos)
{
    const std::size_t start = pos;
    int value = 0;
    while (pos < text.size() && isDigit(text[pos])) {
        value = value * kDecimalBase + (text[pos] - '0');
        ++pos;
    }
    if (pos == start) {
        return std::nullopt;
    }

    return value;
}

std::string_view trimStart(std::string_view text)
{
    while (!text.empty() && text.front() == ' ') {
        text.remove_prefix(1);
    }

    return text;
}

// "error: текст", "warning C4244: текст" (MSVC с кодом) -> уровень и текст.
std::optional<std::pair<log::Level, std::string_view>> readKind(std::string_view text)
{
    text = trimStart(text);
    for (const Kind& kind : kKinds) {
        if (!text.starts_with(kind.word)) {
            continue;
        }
        std::string_view rest = text.substr(kind.word.size());
        // Код MSVC: " C2065", " LNK1104".
        if (!rest.empty() && rest.front() == ' ') {
            const std::size_t colon = rest.find(':');
            if (colon == std::string_view::npos || rest.substr(1, colon - 1).find(' ') != std::string_view::npos) {
                continue;
            }
            rest = rest.substr(colon);
        }
        if (rest.empty() || rest.front() != ':') {
            continue;
        }

        return std::pair{kind.level, trimStart(rest.substr(1))};
    }

    return std::nullopt;
}

// GCC и Clang: "файл:строка:столбец: вид: текст" или без столбца.
std::optional<CompilerDiagnostic> parseGnu(std::string_view line)
{
    for (std::size_t colon = line.find(':'); colon != std::string_view::npos; colon = line.find(':', colon + 1)) {
        if (colon + 1 == kDriveLength) {
            continue;
        }
        std::size_t pos = colon + 1;
        const auto number = readNumber(line, pos);
        if (!number || pos >= line.size() || line[pos] != ':') {
            continue;
        }
        CompilerDiagnostic diagnostic;
        diagnostic.file = line.substr(0, colon);
        diagnostic.line = *number;
        ++pos;
        const std::size_t afterLine = pos;
        if (const auto column = readNumber(line, pos); column && pos < line.size() && line[pos] == ':') {
            diagnostic.column = *column;
            ++pos;
        } else {
            pos = afterLine;
        }
        const auto kind = readKind(line.substr(pos));
        if (!kind) {
            return std::nullopt;
        }
        diagnostic.level = kind->first;
        diagnostic.message = kind->second;

        return diagnostic;
    }

    return std::nullopt;
}

// MSVC: "файл(строка): вид код: текст" или "файл(строка,столбец): ...".
std::optional<CompilerDiagnostic> parseMsvc(std::string_view line)
{
    const std::size_t close = line.find("): ");
    if (close == std::string_view::npos) {
        return std::nullopt;
    }
    const std::size_t open = line.rfind('(', close);
    if (open == std::string_view::npos || open == 0) {
        return std::nullopt;
    }
    CompilerDiagnostic diagnostic;
    std::size_t pos = open + 1;
    const auto number = readNumber(line, pos);
    if (!number) {
        return std::nullopt;
    }
    diagnostic.line = *number;
    if (pos < close && line[pos] == ',') {
        ++pos;
        const auto column = readNumber(line, pos);
        if (!column) {
            return std::nullopt;
        }
        diagnostic.column = *column;
    }
    if (pos != close) {
        return std::nullopt;
    }
    const auto kind = readKind(line.substr(close + 2));
    if (!kind) {
        return std::nullopt;
    }
    diagnostic.file = line.substr(0, open);
    diagnostic.level = kind->first;
    diagnostic.message = kind->second;

    return diagnostic;
}

// Без места: "collect2: error: ...", "LINK : fatal error LNK1104: ...",
// "ninja: error: ...", "CMake Error at ...".
log::Level levelWithoutLocation(std::string_view line)
{
    if (line.starts_with("CMake Error")) {
        return log::Level::Error;
    }
    if (line.starts_with("CMake Warning")) {
        return log::Level::Warning;
    }
    for (std::size_t colon = line.find(':'); colon != std::string_view::npos; colon = line.find(':', colon + 1)) {
        if (const auto kind = readKind(line.substr(colon + 1))) {
            return kind->first;
        }
    }

    return log::Level::Info;
}

} // namespace

CompilerDiagnostic parseCompilerOutput(std::string_view line)
{
    if (auto diagnostic = parseMsvc(line)) {
        return *diagnostic;
    }
    if (auto diagnostic = parseGnu(line)) {
        return *diagnostic;
    }
    CompilerDiagnostic diagnostic;
    diagnostic.level = levelWithoutLocation(line);
    diagnostic.message = line;

    return diagnostic;
}

} // namespace yqlora::editor
