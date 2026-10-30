#include "yqlora/core/AppPaths.hpp"

#include "yqlora/platform/Platform.hpp"

namespace yqlora {

namespace {

constexpr std::string_view kAssetsFolder = "assets";

} // namespace

std::filesystem::path userDataDirectory(std::string_view application)
{
    return platform::userDataPath(kOrganization, application);
}

std::filesystem::path assetsDirectory()
{
    return platform::executableDir() / kAssetsFolder;
}

} // namespace yqlora
