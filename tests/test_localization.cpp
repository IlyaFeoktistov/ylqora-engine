#include <doctest/doctest.h>

#include <yqlora/i18n/Localization.hpp>

#include <string>
#include <vector>

using namespace yqlora;

TEST_CASE("Localization: loads nested keys and falls back to the key itself")
{
    Localization loc(YQLORA_LOCALES_DIR);
    REQUIRE(loc.load("ru"));
    CHECK(loc.language() == "ru");
    CHECK(loc.translate("language.name") == "Русский");
    CHECK(loc.translate("no.such.key") == "no.such.key");

    REQUIRE(loc.load("en"));
    CHECK(loc.translate("language.name") == "English");
}

TEST_CASE("Localization: unknown language keeps the previous one")
{
    Localization loc(YQLORA_LOCALES_DIR);
    REQUIRE(loc.load("en"));
    CHECK_FALSE(loc.load("xx"));
    CHECK(loc.language() == "en");
}

TEST_CASE("Localization: available languages are discovered from files")
{
    Localization loc(YQLORA_LOCALES_DIR);
    const auto languages = loc.availableLanguages();
    CHECK(languages == std::vector<std::string>{"en", "ru"});
}

TEST_CASE("Localization: pickLanguage matches system locales")
{
    const std::vector<std::string> available{"en", "ru"};

    CHECK(Localization::pickLanguage(std::vector<std::string>{"ru_RU"}, available) == "ru");
    CHECK(Localization::pickLanguage(std::vector<std::string>{"de", "ru-RU", "en"}, available) == "ru");
    CHECK(Localization::pickLanguage(std::vector<std::string>{"de"}, available) == "en");
    CHECK(Localization::pickLanguage(std::vector<std::string>{}, available) == "en");
}
