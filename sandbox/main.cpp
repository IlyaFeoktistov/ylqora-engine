#include <yqlora/core/Application.hpp>
#include <yqlora/core/Log.hpp>
#include <yqlora/editor/EditorCamera.hpp>
#include <yqlora/editor/Picking.hpp>
#include <yqlora/editor/Selection.hpp>
#include <yqlora/i18n/Localization.hpp>
#include <yqlora/scene/Camera3D.hpp>
#include <yqlora/scene/MeshInstance3D.hpp>

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

        // Камера игры. Смотрим через камеру редактора, эта просто лежит в сцене.
        auto& camera = root.addChild<yqlora::Camera3D>("MainCamera");
        camera.transform().setPosition({0.0f, 2.0f, 5.0f});
        camera.transform().lookAt({0.0f, 0.0f, 0.0f});
        scene().setActiveCamera(&camera);

        addMesh(root, "Cube", "cube", {0.0f, 0.5f, 0.0f}, {0.85f, 0.45f, 0.3f, 1.0f});
        addMesh(root, "Sphere", "sphere", {1.5f, 0.5f, 0.0f}, {0.35f, 0.65f, 0.9f, 1.0f});
        auto& box = addMesh(root, "Box", "cube", {-1.5f, 0.35f, 0.5f}, {0.5f, 0.8f, 0.45f, 1.0f});
        box.transform().setScale({0.7f, 0.7f, 0.7f});
        box.transform().setRotationDegrees({0.0f, 30.0f, 0.0f});

        m_editorCamera.setPosition({0.0f, 2.0f, 5.0f});
        m_editorCamera.lookAt({0.0f, 0.5f, 0.0f});

        printTree(root);
        yqlora::log::info("Esc - quit, F2 - switch language");
        yqlora::log::info("Left click - select, hold right button - look around, WASD/QE - fly, wheel - speed");
    }

    void onUpdate(float deltaSeconds) override
    {
        const yqlora::Input& in = input();
        if (in.wasKeyPressed(yqlora::Key::Escape)) {
            quit();
        }
        if (in.wasKeyPressed(yqlora::Key::F2)) {
            switchLanguage();
        }

        const bool wasFlying = m_editorCamera.flying();
        m_editorCamera.update(in, deltaSeconds);
        if (m_editorCamera.flying() != wasFlying) {
            setMouseCaptured(m_editorCamera.flying());
        }

        // Клик в пустоту снимает выделение.
        if (!m_editorCamera.flying() && in.wasMouseButtonPressed(yqlora::MouseButton::Left)) {
            const yqlora::Ray ray = m_editorCamera.screenRay(in.mousePosition(), windowSize());
            m_selection.select(yqlora::pickNode(scene(), ray));
        }
    }

    void onPrepareFrame(yqlora::FrameData& frame) override
    {
        frame.camera = m_editorCamera.cameraData(aspectRatio(), depthRange());
        frame.editor.grid = true;
        frame.editor.selected = m_selection.selected();
    }

private:
    yqlora::MeshInstance3D& addMesh(yqlora::Node& parent, std::string name, std::string mesh,
                                    const yqlora::Vec3& position, const yqlora::Vec4& color)
    {
        auto& instance = parent.addChild<yqlora::MeshInstance3D>(std::move(name));
        instance.setMesh(std::move(mesh));
        instance.setColor(color);
        instance.transform().setPosition(position);

        return instance;
    }

    // Следующий язык по кругу из доступных в assets/locales.
    void switchLanguage()
    {
        const auto languages = localization().availableLanguages();
        auto it = std::ranges::find(languages, localization().language());
        it = (it == languages.end() || std::next(it) == languages.end()) ? languages.begin() : std::next(it);
        if (it != languages.end()) {
            setLanguage(*it);
        }
    }

    yqlora::EditorCamera m_editorCamera;
    yqlora::Selection m_selection;
};

} // namespace

int main()
{
    SandboxApp app;

    return app.run();
}
