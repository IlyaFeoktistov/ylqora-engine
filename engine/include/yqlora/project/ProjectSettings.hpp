#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/reflection/TypeName.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace yqlora {

class TypeRegistry;

// Настройки проекта: по экземпляру каждого раздела из реестра, у каждого
// свой файл settings/<раздел>.json в папке проекта. Файлы читаются один раз
// при открытии проекта, пишутся по одному при правке раздела.
class YQLORA_API ProjectSettings {
public:
    ProjectSettings() = default;
    ProjectSettings(ProjectSettings&&) = default;
    ProjectSettings& operator=(ProjectSettings&&) = default;
    ProjectSettings(const ProjectSettings&) = delete;
    ProjectSettings& operator=(const ProjectSettings&) = delete;

    static constexpr std::string_view directoryName()
    {
        return "settings";
    }

    static constexpr std::string_view fileExtension()
    {
        return ".json";
    }

    // Создаёт разделы со значениями по умолчанию и читает их файлы. Нет
    // файла - значения по умолчанию. Файлы разделов, которых в реестре нет
    // (модуль не загрузился), не трогаются, поэтому их значения не теряются.
    // Реестр должен пережить настройки: по нему разделы пишутся в файлы.
    void load(const TypeRegistry& registry, const std::filesystem::path& projectDirectory);

    // Пишет файл одного раздела.
    bool save(const SettingsSection& section) const;

    // nullptr, если такого раздела нет.
    SettingsSection* find(std::string_view typeName);
    const SettingsSection* find(std::string_view typeName) const;

    // Раздел типа T; он должен быть зарегистрирован.
    template <std::derived_from<SettingsSection> T>
    T& get()
    {
        SettingsSection* section = find(typeNameOf<T>());
        assert(section && "ProjectSettings: the section is not registered");

        return static_cast<T&>(*section);
    }

    template <std::derived_from<SettingsSection> T>
    const T& get() const
    {
        const SettingsSection* section = find(typeNameOf<T>());
        assert(section && "ProjectSettings: the section is not registered");

        return static_cast<const T&>(*section);
    }

    // Разделы в порядке регистрации их типов.
    const std::vector<std::unique_ptr<SettingsSection>>& sections() const
    {
        return m_sections;
    }

private:
    std::filesystem::path file(const SettingsSection& section) const;

    const TypeRegistry* m_registry = nullptr;
    std::filesystem::path m_directory;
    std::vector<std::unique_ptr<SettingsSection>> m_sections;
};

} // namespace yqlora
