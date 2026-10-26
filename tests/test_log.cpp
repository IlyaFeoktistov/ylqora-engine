#include <doctest/doctest.h>

#include <yqlora/core/LogSettings.hpp>

using namespace yqlora;

TEST_CASE("Log: only warnings and errors by default, everything in debug")
{
    CHECK_FALSE(log::isEnabled(log::Level::Info));
    CHECK(log::isEnabled(log::Level::Warning));
    CHECK(log::isEnabled(log::Level::Error));

    log::setMinimumLevel(log::Level::Info);
    CHECK(log::isEnabled(log::Level::Info));

    log::setMinimumLevel(log::Level::Warning);
}
