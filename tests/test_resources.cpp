#include <doctest/doctest.h>

#include <yqlora/resource/BuiltinResources.hpp>
#include <yqlora/resource/ResourceDatabase.hpp>

#include <array>
#include <filesystem>
#include <fstream>
#include <string_view>

using namespace yqlora;

namespace {

// Как файл проекта: в корне, но не ресурс.
constexpr std::array<std::string_view, 1> kNotResources{"project.yqlora"};

std::filesystem::path freshDirectory(const char* name)
{
    const auto directory = std::filesystem::temp_directory_path() / "yqlora_tests" / name;
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);

    return directory;
}

void touch(const std::filesystem::path& file)
{
    std::filesystem::create_directories(file.parent_path());
    std::ofstream(file) << "data";
}

} // namespace

TEST_CASE("ResourceDatabase: files get ids that survive a rescan")
{
    const auto root = freshDirectory("resources_scan");
    touch(root / "player.png");
    touch(root / "levels" / "one.yscene");
    touch(root / ".hidden" / "secret.txt");
    touch(root / "project.yqlora");

    ResourceDatabase database;
    database.scan(root, kNotResources);
    CHECK(database.size() == 2);
    const ResourceId player = database.id("player.png");
    const ResourceId level = database.id("levels/one.yscene");
    CHECK(player != 0);
    CHECK_FALSE(builtin::isBuiltin(player));
    CHECK(level != player);
    CHECK(std::filesystem::exists(root / "player.png.meta"));
    CHECK(database.id("project.yqlora") == 0);

    ResourceDatabase again;
    again.scan(root, kNotResources);
    CHECK(again.id("player.png") == player);
    CHECK(again.path(level) == std::filesystem::path("levels/one.yscene"));
}

TEST_CASE("ResourceDatabase: a copied file with its .meta gets a new id")
{
    const auto root = freshDirectory("resources_copy");
    touch(root / "a.png");
    ResourceDatabase database;
    database.scan(root);
    std::filesystem::copy_file(root / "a.png", root / "b.png");
    std::filesystem::copy_file(root / "a.png.meta", root / "b.png.meta");

    database.scan(root);
    CHECK(database.size() == 2);
    CHECK(database.id("a.png") != database.id("b.png"));
}

TEST_CASE("ResourceDatabase: ids follow renamed files and folders, removed ones are forgotten")
{
    const auto root = freshDirectory("resources_move");
    touch(root / "art" / "hero.png");
    touch(root / "notes.txt");
    ResourceDatabase database;
    database.scan(root);
    const ResourceId hero = database.id("art/hero.png");
    const ResourceId notes = database.id("notes.txt");

    std::filesystem::rename(root / "notes.txt", root / "readme.txt");
    database.moved("notes.txt", "readme.txt");
    CHECK(database.id("readme.txt") == notes);
    CHECK(std::filesystem::exists(root / "readme.txt.meta"));
    CHECK_FALSE(std::filesystem::exists(root / "notes.txt.meta"));

    std::filesystem::rename(root / "art", root / "sprites");
    database.moved("art", "sprites");
    CHECK(database.path(hero) == std::filesystem::path("sprites/hero.png"));

    std::filesystem::remove_all(root / "sprites");
    database.removed("sprites");
    CHECK_FALSE(database.path(hero));

    // Новый файл получает ID сразу, без повторного обхода.
    touch(root / "new.png");
    const ResourceId created = database.add("new.png");
    CHECK(created != 0);
    CHECK(database.add("new.png") == created);
}
