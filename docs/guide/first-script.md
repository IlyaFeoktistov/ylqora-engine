# Your first script

[Русская версия](first-script.ru.md)

A script is a C++ class attached to a node as a component. Scripts are files
of the project, like scenes, and can live in any folder. The editor builds
all of them into one shared library, the game module (`game.dll` on Windows,
`libgame.so` on Linux), with the same compiler as the engine, and reloads it
while you work. The project has no build files of its own.

## 1. Create the script

In Files press "Create resource" (or right-click a folder) and pick
"C++ script". Enter the class name, for example `Spinner`. The editor
creates `Spinner.hpp` and `Spinner.cpp` in the folder and builds the module;
the Log panel shows the build.

A double click on `Spinner.cpp` opens it in the program your system uses
for C++ files (your IDE).

## 2. Write the code

`Spinner.hpp`:

```cpp
#pragma once

#include <yqlora/scripting/Script.hpp>

class Spinner : public yqlora::Script {
public:
    YQLORA_SCRIPT(Spinner)

    float degreesPerSecond = 90.0f;

    YQLORA_FIELDS(field("degreesPerSecond", &Spinner::degreesPerSecond).range(-720.0f, 720.0f))

protected:
    void onUpdate(float deltaSeconds) override;
};
```

`Spinner.cpp`:

```cpp
#include "Spinner.hpp"

#include <yqlora/scene/Node3D.hpp>
#include <yqlora/scene/Transform3D.hpp>
#include <yqlora/scripting/ModuleScripts.hpp>

#include <glm/gtc/quaternion.hpp>

YQLORA_REGISTER_SCRIPT(Spinner).requiresNode<yqlora::Node3D>();

void Spinner::onUpdate(float deltaSeconds)
{
    const yqlora::Quat turn = glm::angleAxis(glm::radians(degreesPerSecond * deltaSeconds), yqlora::math::kWorldUp);
    transform().setRotation(transform().rotation() * turn);
}
```

- `YQLORA_SCRIPT(Spinner)` sets the type name; scene files store it.
- `YQLORA_FIELDS` lists the fields the inspector shows and scene files
  store, with their attributes (`.range`, `.tooltip`, ...). Other fields
  live only while the game runs.
- `YQLORA_REGISTER_SCRIPT` adds the script to the game module, with rules
  after it: `requiresNode`, `requiresComponent`, `formerly`,
  `executionOrder`. No other file lists the scripts.
- Lifecycle calls (`onStart`, `onUpdate`, ...) and their order are in
  [scripts](../architecture/scripting.md#lifecycle-contract).

Save and switch back to the editor: it builds the module and reloads it.
Ctrl+B (Scripts > Build) builds at any time, and Play builds first too.

## 3. Use it

Select the cube, press "Add component": `Spinner` is under "Scripts".
Change `degreesPerSecond` in the inspector, press Play.

Change the code again and come back: the scene keeps the values of the
script fields; the edit history is cleared, because its commands hold
objects of the old code. Compiler errors are red in the Log panel, the main
menu marks a failed build, and the old module keeps working until the next
successful build.

## Limits

- Scripts are native code in the editor process. An exception thrown from a
  lifecycle call disables the component and goes to the log, but a crash
  (a null pointer) takes the editor down with it, and an endless loop hangs
  it. Save before Play.
- One game module per project. A script whose name is already taken (by an
  engine type or by another script) is not registered; the log says which
  module holds the name.
- A field name in `field("name", ...)` is stored in scene files. Renaming
  it, mark the old one: `.formerly("oldName")`; renaming the class,
  `YQLORA_REGISTER_SCRIPT(T).formerly("OldName")`.
- If the module is not built, scripts in the scenes are kept as
  placeholders: the inspector shows them as missing, saving writes their
  data back unchanged, and they come back with the next successful build.
- Files in hidden folders (name starting with a dot) are not built.
- The editor builds with the compiler the engine was built with
  (`sdk/toolchain.json` next to the editor). An engine built with MSVC
  cannot build scripts from the editor yet.
