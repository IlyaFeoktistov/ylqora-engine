#pragma once

#include "SceneTreePanel.hpp"
#include "ViewportPanel.hpp"

#include "yqlora/core/Application.hpp"
#include "yqlora/editor/Selection.hpp"

#include <string>
#include <utility>
#include <vector>

namespace yqlora {

class EditorApp final : public Application {
public:
    EditorApp();

protected:
    void onStart() override;
    void onUpdate(float deltaSeconds) override;
    void onGui() override;

private:
    void drawMenuBar();
    void drawLanguageMenu();
    // При первом запуске раскладывает окна: дерево слева, инспектор справа,
    // файлы снизу, сцена в центре. Дальше раскладку помнит imgui.ini.
    void buildDefaultLayout(unsigned int dockspace);

    Selection m_selection;
    SceneTreePanel m_sceneTree;
    ViewportPanel m_viewport;
    float m_deltaSeconds = 0.0f;
    // Языки с названиями. Названия читаются из файлов, поэтому один раз при старте.
    std::vector<std::pair<std::string, std::string>> m_languages;
    bool m_layoutChecked = false;
};

} // namespace yqlora
