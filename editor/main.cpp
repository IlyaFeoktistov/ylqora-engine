#include "EditorApp.hpp"

#include "yqlora/platform/Platform.hpp"

#include <string_view>

int main(int argc, char** argv)
{
    // Программа запускается без консоли, лог пишется в файл в папке настроек.
    // С флагом --debug лог виден ещё и в консоли.
    for (int i = 1; i < argc; ++i) {
        if (std::string_view(argv[i]) == "--debug") {
            yqlora::platform::openConsole();
        }
    }

    yqlora::EditorApp app;

    return app.run();
}
