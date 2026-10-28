#include "yqlora/core/FileFormat.hpp"

#include "yqlora/core/Log.hpp"

namespace yqlora {

namespace {

constexpr const char* kVersionKey = "version";

} // namespace

FormatCompatibility FileFormat::compatibility(int version) const
{
    if (version == current()) {
        return FormatCompatibility::Current;
    }
    if (version > current()) {
        return FormatCompatibility::Newer;
    }

    return version >= m_first ? FormatCompatibility::Older : FormatCompatibility::Unsupported;
}

int FileFormat::versionOf(const nlohmann::json& document)
{
    const auto it = document.find(kVersionKey);

    return it != document.end() && it->is_number_integer() ? it->get<int>() : 0;
}

void FileFormat::stamp(nlohmann::json& document) const
{
    document[kVersionKey] = current();
}

bool FileFormat::migrate(nlohmann::json& document) const
{
    if (!document.contains(kVersionKey)) {
        log::error("{}: no format version", m_name);

        return false;
    }
    const int version = versionOf(document);
    const FormatCompatibility compatibility = this->compatibility(version);
    if (compatibility == FormatCompatibility::Newer || compatibility == FormatCompatibility::Unsupported) {
        log::error("{}: format version {} is not supported, this engine reads up to {}", m_name, version, current());

        return false;
    }
    for (int from = version; from < current(); ++from) {
        m_migrations[static_cast<std::size_t>(from - m_first)](document);
    }
    stamp(document);

    return true;
}

} // namespace yqlora
