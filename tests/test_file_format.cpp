#include <doctest/doctest.h>

#include <yqlora/core/FileFormat.hpp>

#include <array>

using namespace yqlora;
using Json = nlohmann::json;

namespace {

// Версия 1 звала поле "title", версия 2 - "name".
void renameTitle(Json& document)
{
    document["name"] = document["title"];
    document.erase("title");
}

// Версия 3 добавила поле со значением по умолчанию.
void addSpeed(Json& document)
{
    document["speed"] = 1;
}

constexpr std::array<Migration, 2> kMigrations{renameTitle, addSpeed};
constexpr FileFormat kFormat{"Test", 1, kMigrations};

} // namespace

TEST_CASE("FileFormat: the current version follows the migrations")
{
    CHECK(kFormat.current() == 3);
    CHECK(kFormat.compatibility(3) == FormatCompatibility::Current);
    CHECK(kFormat.compatibility(1) == FormatCompatibility::Older);
    CHECK(kFormat.compatibility(4) == FormatCompatibility::Newer);
    CHECK(kFormat.compatibility(0) == FormatCompatibility::Unsupported);
}

TEST_CASE("FileFormat: migrations run in order up to the current version")
{
    Json document = {{"version", 1}, {"title", "Game"}};
    REQUIRE(kFormat.migrate(document));
    CHECK(document == Json{{"version", 3}, {"name", "Game"}, {"speed", 1}});

    // С середины цепочки - только оставшиеся переводы.
    Json middle = {{"version", 2}, {"name", "Game"}};
    REQUIRE(kFormat.migrate(middle));
    CHECK(middle == Json{{"version", 3}, {"name", "Game"}, {"speed", 1}});
}

TEST_CASE("FileFormat: unknown versions are not read")
{
    Json newer = {{"version", 4}};
    CHECK_FALSE(kFormat.migrate(newer));
    CHECK(newer == Json{{"version", 4}});

    Json none = Json::object();
    CHECK_FALSE(kFormat.migrate(none));
    CHECK(FileFormat::versionOf(none) == 0);
}
