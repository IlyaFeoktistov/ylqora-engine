# Your first script

[Русская версия](first-script.ru.md)

A script is a C++ class attached to a node as a component. Scripts of a game
live in one shared library, the game module: `game.dll` on Windows,
`libgame.so` on Linux. The editor loads it from the `.build/` folder of the
project when the project opens.

Until step 3 (building scripts from the editor) the game module is built
inside the engine's CMake tree, next to the examples. This guide makes a
`Pulse` script that makes its node grow and shrink.

## 1. Write the script

Create `examples/my-game/Pulse.cpp`:

```cpp
#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Transform3D.hpp>
#include <yqlora/scripting/ModuleRegistrar.hpp>
#include <yqlora/scripting/Script.hpp>

#include <glm/gtc/constants.hpp>

#include <cmath>

class Pulse : public yqlora::Script {
public:
    YQLORA_SCRIPT(Pulse)

    float amplitude = 0.2f;
    float frequency = 1.0f;

    YQLORA_FIELDS(
        field("amplitude", &Pulse::amplitude).range(0.0f, 1.0f).tooltip("How much the node grows and shrinks"),
        field("frequency", &Pulse::frequency).range(0.0f, 10.0f).tooltip("Pulses per second")
    )

protected:
    void onStart() override
    {
        m_baseScale = transform().scale();
    }

    void onUpdate(float deltaSeconds) override
    {
        m_elapsed += deltaSeconds;
        const float wave = std::sin(m_elapsed * frequency * glm::two_pi<float>());
        transform().setScale(m_baseScale * (1.0f + amplitude * wave));
    }

private:
    // Не перечислены в YQLORA_FIELDS: живут только во время игры.
    yqlora::Vec3 m_baseScale{1.0f};
    float m_elapsed = 0.0f;
};

YQLORA_MODULE_ENTRY(registrar)
{
    registrar.registerScript<Pulse>().requiresNode<yqlora::Node3D>();
}
```

- `YQLORA_SCRIPT(Pulse)` names the type; the name is what scene files keep.
- `YQLORA_FIELDS` lists the fields the inspector shows and scene files save,
  with their attributes. Fields not listed (here `m_baseScale`, `m_elapsed`)
  live only while the game runs.
- `YQLORA_MODULE_ENTRY` is the one place where the library registers its
  scripts. A module has exactly one entry function.
- The lifecycle calls (`onStart`, `onUpdate`, ...) and their order are
  described in [scripts](../architecture/scripting.md#lifecycle-contract).

## 2. Build the library

Create `examples/my-game/CMakeLists.txt`:

```cmake
add_library(my_game SHARED Pulse.cpp)
target_link_libraries(my_game PRIVATE yqlora::yqlora)
set_target_properties(my_game PROPERTIES
    OUTPUT_NAME game
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/my-game
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples/my-game
)
if(WIN32)
    set_target_properties(my_game PROPERTIES PREFIX "")
endif()
```

Add `add_subdirectory(my-game)` to `examples/CMakeLists.txt` and build:

```bash
cmake --preset debug
cmake --build --preset debug
```

The library appears in `build/debug/examples/my-game/`.

## 3. Put it into the project

Copy `game.dll` (`libgame.so`) into the `.build/` folder of your project,
next to `project.yqlora`. The folder is hidden from the Files panel and is
never a resource.

## 4. Use it in the editor

Open the project. Select a node, press "Add component": `Pulse` is under
"Scripts". Change `amplitude` and `frequency` in the inspector, press Play.

To change the code: close the project (on Windows the loaded library cannot
be replaced), build, copy the library again, open the project. Hot reload
comes in step 3.

If the project opens without the library, scripts in its scenes are kept as
placeholders: the inspector shows them as missing, saving writes their data
back unchanged, and they come back when the library is there again.

## Limits

- Scripts are native code in the editor process. An exception thrown from a
  lifecycle call disables the component and goes to the log, but a crash
  (a null pointer) takes the editor down with it, and an endless loop hangs
  it. Save before Play.
- One game module per project. A script whose name is already taken (by an
  engine type or by another script) is not registered; the log says which module holds the name.
- A field name in `field("name", ...)` is stored in scene files. Renaming
  it, mark the old one: `.formerly("oldName")`; renaming the class,
  `registerScript<T>().formerly("OldName")`.
