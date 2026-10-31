#include "yqlora/scripting/GameModule.hpp"

#include "yqlora/core/Log.hpp"
#include "yqlora/core/Utf8Path.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/Scene.hpp"

#include <cassert>
#include <string>

namespace yqlora {

namespace {

constexpr const char* kBuildDirectory = ".build";
constexpr const char* kLibraryName = "game";

void destroyModuleComponents(Scene& scene, const TypeRegistry& registry, std::string_view module)
{
    assert(!scene.inFrame() && "GameModule::unload: not during a frame of the scene");
    for (const auto& type : registry.types()) {
        if (type->module() != module || type->kind() != TypeKind::Component) {
            continue;
        }
        // Копия: снятый компонент уходит из индекса сцены.
        const std::span<Component* const> live = scene.componentsOfType(type->name());
        const std::vector<Component*> components(live.begin(), live.end());
        for (Component* component : components) {
            [[maybe_unused]] const auto removed = component->owner()->removeComponent(*component);
            assert(removed && "GameModule::unload: a component of the module cannot be removed");
        }
    }
}

} // namespace

std::filesystem::path gameLibraryPath(const std::filesystem::path& projectDirectory)
{
    return projectDirectory / kBuildDirectory / platform::sharedLibraryFileName(kLibraryName);
}

GameModule::GameModule(TypeRegistry& registry)
    : m_registry(registry)
{
}

GameModule::~GameModule()
{
    unload({});
}

bool GameModule::load(const std::filesystem::path& library)
{
    assert(!loaded() && "GameModule::load: unload the previous library first");
    auto opened = std::make_unique<platform::SharedLibrary>(library);
    if (!opened->loaded()) {
        log::error("Game module '{}' is not loaded: {}", toUtf8(library), opened->error());

        return false;
    }
    const auto entry = reinterpret_cast<ModuleEntry>(opened->function(kModuleEntrySymbol));
    if (!entry) {
        log::error("Game module '{}' has no {} (YQLORA_MODULE_ENTRY)", toUtf8(library), kModuleEntrySymbol);

        return false;
    }

    m_library = std::move(opened);
    ModuleRegistrar registrar(m_registry, std::string(moduleName()));
    entry(registrar);
    m_rejected.assign(registrar.rejected().begin(), registrar.rejected().end());
    for (const RejectedType& type : m_rejected) {
        log::error(
            "Game module: type '{}' is not registered, the name is taken by module '{}'",
            type.name,
            type.takenBy
        );
    }

    return true;
}

void GameModule::unload(std::span<Scene* const> scenes)
{
    if (!loaded()) {
        return;
    }
    for (Scene* scene : scenes) {
        destroyModuleComponents(*scene, m_registry, moduleName());
    }
    // Функции типов (фабрики, доступ к полям) - код библиотеки: уходят до неё.
    m_registry.removeModule(moduleName());
    m_rejected.clear();
    m_library.reset();
}

} // namespace yqlora
