#include <doctest/doctest.h>

#include <yqlora/core/Settings.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

using namespace yqlora;

namespace {

std::filesystem::path tempSettingsFile(const char* name)
{
    const auto dir = std::filesystem::temp_directory_path() / "yqlora_tests";
    std::filesystem::create_directories(dir);
    const auto file = dir / name;
    std::filesystem::remove(file);
    return file;
}

std::string readAll(const std::filesystem::path& file)
{
    std::ifstream in(file);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

} // namespace

TEST_CASE("Settings: missing file loads defaults")
{
    Settings settings(tempSettingsFile("missing.json"));
    CHECK_FALSE(settings.load());
    CHECK(settings.language.empty());
}

TEST_CASE("Settings: language survives save and load")
{
    const auto file = tempSettingsFile("roundtrip.json");
    {
        Settings settings(file);
        settings.language = "ru";
        REQUIRE(settings.save());
    }
    Settings loaded(file);
    REQUIRE(loaded.load());
    CHECK(loaded.language == "ru");
}

TEST_CASE("Settings: save keeps unknown keys")
{
    const auto file = tempSettingsFile("unknown_keys.json");
    {
        std::ofstream out(file);
        out << R"({"language": "en", "graphics": {"vsync": true}})";
    }
    Settings settings(file);
    REQUIRE(settings.load());
    settings.language = "ru";
    REQUIRE(settings.save());

    const auto content = readAll(file);
    CHECK(content.find("\"vsync\"") != std::string::npos);
    CHECK(content.find("\"ru\"") != std::string::npos);
}

TEST_CASE("Settings: corrupted file does not crash")
{
    const auto file = tempSettingsFile("corrupted.json");
    {
        std::ofstream out(file);
        out << "{ not json";
    }
    Settings settings(file);
    CHECK_FALSE(settings.load());
    CHECK(settings.language.empty());
}
