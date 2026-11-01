#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/project/SettingsSection.hpp"
#include "yqlora/reflection/Property.hpp"

#include <string>

namespace yqlora {

// Общие сведения об игре.
class YQLORA_API GeneralSettings final : public SettingsSection {
public:
    YQLORA_TYPE(GeneralSettings, SettingsSection)

    // Версия новой игры. Это версия для игроков, её задаёт разработчик игры;
    // версия формата проекта - другое (Project::formatVersion).
    static constexpr std::string_view defaultGameVersion()
    {
        return "0.0.1";
    }

    std::string gameName;
    std::string gameVersion{defaultGameVersion()};
    std::string company;
    // Сцена, которая открывается вместе с проектом и запускается в игре.
    ResourceRef startupScene;
    // Иконка окна игры (и позже исполняемого файла): текстура проекта.
    ResourceRef icon;
};

// Имя свойства startupScene в реестре: по нему редактор ставит стартовую сцену командой с отменой.
inline constexpr char kStartupSceneProperty[] = "startupScene";

} // namespace yqlora
