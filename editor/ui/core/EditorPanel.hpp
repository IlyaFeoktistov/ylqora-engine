#pragma once

#include "core/EditorContext.hpp"

#include "yqlora/i18n/Translate.hpp"

#include <string>

namespace yqlora::editor {

// Окно редактора, которое можно пристыковать: дерево, инспектор, файлы, вид.
class EditorPanel {
public:
    virtual ~EditorPanel() = default;

    // Имя окна в imgui.ini и в раскладке, от языка не зависит.
    virtual const char* id() const = 0;
    virtual const char* titleKey() const = 0;

    // Окно, которое нельзя закрыть, не показывается в меню "Вид".
    virtual bool closable() const
    {
        return true;
    }

    virtual void draw(EditorContext& context) = 0;

    virtual void onProjectOpened(EditorContext& /*context*/)
    {
    }

    bool visible() const
    {
        return m_visible;
    }

    void setVisible(bool visible)
    {
        m_visible = visible;
    }

    std::string title() const
    {
        return tr(titleKey()) + "###" + id();
    }

protected:
    // Для ImGui::Begin: крестик в заголовке, если окно можно закрыть.
    bool* visibleFlag()
    {
        return closable() ? &m_visible : nullptr;
    }

private:
    bool m_visible = true;
};

} // namespace yqlora::editor
