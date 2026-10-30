#pragma once

#include "yqlora/editor/ui/core/EditorContext.hpp"

#include "yqlora/i18n/Translate.hpp"

#include <imgui.h>

#include <string>

namespace yqlora::editor {

// Окно редактора, которое можно пристыковать: дерево, инспектор, файлы, вид.
// Любое окно можно развернуть на всё окно редактора и вернуть обратно
// (Shift+Пробел, правый клик или двойной клик по вкладке), раскладка при этом
// не меняется.
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

    // Рендерер сейчас пересоздадут: всё, что окно создало через него, надо отпустить.
    virtual void onRendererLost()
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

    // Окно в фокусе в прошлом кадре: его разворачивает Shift+Пробел.
    bool focused() const
    {
        return m_focused;
    }

protected:
    // Открывает окно панели, как ImGui::Begin; ImGui::End вызывать всегда.
    // Развёрнутая панель - отдельное окно на всю рабочую область, у
    // свёрнутой в заголовке или на вкладке меню "Развернуть".
    bool beginWindow(EditorContext& context, ImGuiWindowFlags flags = 0);

    bool maximized(const EditorContext& context) const
    {
        return context.maximized == this;
    }

    // Для ImGui::Begin: крестик в заголовке, если окно можно закрыть.
    bool* visibleFlag()
    {
        return closable() ? &m_visible : nullptr;
    }

private:
    bool m_visible = true;
    bool m_focused = false;
};

} // namespace yqlora::editor
