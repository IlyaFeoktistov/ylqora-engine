#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/platform/SharedLibrary.hpp"
#include "yqlora/scripting/ModuleRegistrar.hpp"

#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace yqlora {

class Scene;
class TypeRegistry;

// Библиотека игры (game.dll, libgame.so) со скриптами: загружается,
// регистрирует свои типы входной функцией (YQLORA_MODULE_ENTRY) и
// выгружается. Модуль игры у проекта один.
class YQLORA_API GameModule {
public:
    // Модуль типов игры в реестре.
    static constexpr std::string_view moduleName()
    {
        return "game";
    }

    explicit GameModule(TypeRegistry& registry);
    // Если модуль ещё загружен, выгружает без сцен: экземпляров его типов к
    // этому моменту быть не должно.
    ~GameModule();

    GameModule(const GameModule&) = delete;
    GameModule& operator=(const GameModule&) = delete;

    // Загружает библиотеку и регистрирует её типы. false, если библиотека не
    // загрузилась или в ней нет входной функции (причина в логе). Тип с занятым
    // именем не регистрируется, остальные работают; отклонённые в rejected() и в логе.
    bool load(const std::filesystem::path& library);

    // Уничтожает каждый компонент типов модуля в сценах scenes (с onDisable и
    // onStop, пока код ещё загружен), убирает типы из реестра и выгружает
    // библиотеку. Не во время кадра сцены. Экземпляры вне этих сцен (например,
    // в истории правок) владелец уничтожает сам до вызова.
    void unload(std::span<Scene* const> scenes);

    bool loaded() const
    {
        return m_library != nullptr;
    }

    std::span<const RejectedType> rejected() const
    {
        return m_rejected;
    }

private:
    TypeRegistry& m_registry;
    std::unique_ptr<platform::SharedLibrary> m_library;
    std::vector<RejectedType> m_rejected;
};

// Библиотека игры проекта: <проект>/.build/game.dll (libgame.so). Скрытая
// папка, поэтому обход ресурсов её не видит; в шаге 3 туда же собирает редактор.
YQLORA_API std::filesystem::path gameLibraryPath(const std::filesystem::path& projectDirectory);

} // namespace yqlora
