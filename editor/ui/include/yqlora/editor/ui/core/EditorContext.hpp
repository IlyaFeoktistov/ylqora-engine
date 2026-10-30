#pragma once

#include <filesystem>
#include <functional>
#include <string_view>

namespace yqlora {

class SceneSerializer;
class TypeRegistry;

} // namespace yqlora

namespace yqlora::editor {

class ActionRegistry;
class EditorHost;
class EditorPanel;
class EditorSession;
class EditorSettings;
class Theme;

// Всё, с чем работают панели редактора. Собирается заново каждый кадр.
struct EditorContext {
    const EditorHost& host;
    EditorSession& session;
    const TypeRegistry& registry;
    const SceneSerializer& serializer;
    ActionRegistry& actions;
    const Theme& theme;
    EditorSettings& settings;
    float deltaSeconds = 0.0f;

    // Открыть сцену проекта. Сначала спросит про несохранённые изменения в текущей.
    std::function<void(const std::filesystem::path&)> openScene;
    // Открыть окно настроек проекта на разделе (имя типа раздела).
    std::function<void(std::string_view section)> openProjectSettings;
    // Панель, развёрнутая на всё окно редактора; nullptr - обычная раскладка.
    const EditorPanel* maximized = nullptr;
    // Развернуть панель на всё окно или вернуть раскладку, если она уже развёрнута.
    std::function<void(EditorPanel&)> toggleMaximized;

    // Панель ставит, если в этом кадре курсору нужен перенос у края окна:
    // летит камера или тянут значение.
    bool mouseWrapping = false;
};

} // namespace yqlora::editor
