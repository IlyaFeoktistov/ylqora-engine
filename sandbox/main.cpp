#include <yqlora/core/Application.hpp>
#include <yqlora/core/Log.hpp>
#include <yqlora/i18n/Localization.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/Node3D.hpp>

#include <algorithm>
#include <iterator>

namespace {

void printTree(const yqlora::Node& node, int depth = 0)
{
    yqlora::log::info("{:{}}- {} ({})", "", depth * 2, node.name(), node.typeName());
    for (const auto& child : node.children()) {
        printTree(*child, depth + 1);
    }
}

class SandboxApp final : public yqlora::Application {
public:
    SandboxApp()
        : Application({.name = "Sandbox"})
    {
    }

protected:
    void onStart() override
    {
        auto& root = scene().createRoot<yqlora::Node>("Root");

        // Пустой объект: центр сцены, вокруг которого будет летать камера.
        m_pivot = &root.addChild<yqlora::Node3D>("Pivot");

        m_camera = &m_pivot->addChild<yqlora::Camera3D>("MainCamera");
        m_camera->transform().setPosition({0.0f, 2.0f, 5.0f});
        m_camera->transform().lookAt({0.0f, 0.0f, 0.0f});
        scene().setActiveCamera(m_camera);

        printTree(root);
        yqlora::log::info("Esc - quit, F2 - switch language");
    }

    void onUpdate(float deltaSeconds) override
    {
        // Вращаем пустой объект, камера как его ребёнок облетает центр.
        m_angle += deltaSeconds * 30.0f;
        m_pivot->transform().setRotationDegrees({0.0f, m_angle, 0.0f});
    }

    void onPrepareFrame(yqlora::FrameData& frame) override
    {
        frame.editor.grid = true;
    }

    void onKeyPressed(yqlora::Key key) override
    {
        switch (key) {
        case yqlora::Key::Escape:
            quit();
            break;
        case yqlora::Key::F2: {
            // Следующий язык по кругу из доступных в assets/locales.
            const auto languages = localization().availableLanguages();
            auto it = std::ranges::find(languages, localization().language());
            it = (it == languages.end() || std::next(it) == languages.end()) ? languages.begin() : std::next(it);
            if (it != languages.end()) {
                setLanguage(*it);
            }
            break;
        }
        default:
            break;
        }
    }

private:
    yqlora::Node3D* m_pivot = nullptr;
    yqlora::Camera3D* m_camera = nullptr;
    float m_angle = 0.0f;
};

} // namespace

int main()
{
    SandboxApp app;

    return app.run();
}
