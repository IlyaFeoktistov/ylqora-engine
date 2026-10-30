#include "yqlora/editor/ui/EditorApp.hpp"

#include "yqlora/core/LogSettings.hpp"
#include "yqlora/platform/Platform.hpp"

#include <string_view>

int main(int argc, char** argv)
{
    // Обычный запуск: без консоли, в файл лога в папке настроек попадают только
    // предупреждения и ошибки. С флагом --debug пишется всё и открывается консоль.
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--debug") {
            yqlora::log::setMinimumLevel(yqlora::log::Level::Info);
            yqlora::platform::openConsole();
        }
    }

    yqlora::editor::EditorApp app;

    return app.run();
}
