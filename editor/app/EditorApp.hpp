#pragma once

#include "ProjectManagerPanel.hpp"
#include "SceneTreePanel.hpp"
#include "ViewportPanel.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/Project.hpp"
#include "yqlora/editor/ProjectList.hpp"
#include "yqlora/editor/Selection.hpp"
#include "yqlora/reflection/TypeRegistry.hpp"
#include "yqlora/scene/SceneSerializer.hpp"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace yqlora {

// Движок с редактором. При запуске показывает менеджер проектов,
// после выбора проекта - редактор его сцены.
class EditorApp final : public Application {
public:
    EditorApp();

protected:
    void onStart() override;
    void onUpdate(float deltaSeconds) override;
    void onGui() override;

private:
    void openProject(const Project& project);
    void closeProject();
    void saveScene();

    void drawMenuBar();
    void drawLanguageMenu();
    // При первом запуске раскладывает окна: дерево слева, инспектор справа,
    // файлы снизу, сцена в центре. Дальше раскладку помнит imgui.ini.
    void buildDefaultLayout(unsigned int dockspace);

    TypeRegistry m_registry;
    SceneSerializer m_serializer{m_registry};
    std::unique_ptr<ProjectList> m_projects;
    std::unique_ptr<ProjectManagerPanel> m_projectManager;
    std::optional<Project> m_project;

    Selection m_selection;
    SceneTreePanel m_sceneTree;
    ViewportPanel m_viewport;
    float m_deltaSeconds = 0.0f;
    bool m_layoutChecked = false;
    // Языки с названиями. Названия читаются из файлов, поэтому один раз при старте.
    std::vector<std::pair<std::string, std::string>> m_languages;
};

} // namespace yqlora
