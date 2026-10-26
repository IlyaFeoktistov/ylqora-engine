# Yqlora Engine

[Русская версия](README.ru.md)

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows-lightgrey.svg)

A 3D game engine written in C++20, with a Godot-style scene tree, components on nodes
and a renderer hidden behind an API-agnostic interface.

> **Status:** early development. The engine core is in place; the editor is next.

## Features

- **Scene tree** in the spirit of Godot: `Node`, `Node3D`, `Camera3D`. A scene has a single
  root of any node type and can be empty.
- **Components on nodes.** Every `Node3D` has a built-in `Transform3D` that cannot be removed;
  a plain `Node` has no transform and is used for logic and grouping.
- **Type registry with properties.** Nodes and components are created by type name and expose
  their editable properties. This is the foundation for the editor inspector and scene files.
- **Scene files** in JSON with stable node IDs. Removed components and properties are skipped
  with a warning, so old scenes keep loading.
- **Public API boundary.** Game code sees only `engine/include/`; windowing, platform and
  rendering live in `engine/internal/`, and CMake enforces the split.
- **Pluggable renderer.** OpenGL 3.3 backend today; Vulkan and Direct3D 12 are planned.
- **Editor visuals:** an infinite grid that fades with distance and a pixel-wide outline
  around the selected object.
- **Editor camera and picking.** A fly camera that lives outside the scene, and object
  selection by clicking, precise to the triangle.
- **Localization** of the engine UI (English, Russian).
- **Unit tests** with doctest.

## Roadmap

MVP: a minimal editor in the spirit of Godot.

- [x] Scene tree, components, transforms
- [x] Type registry and scene serialization
- [x] Renderer basics: shaders, primitive meshes, infinite editor grid, selection outline
- [ ] Offscreen render targets for the editor viewport
- [x] Input: keyboard, mouse, wheel; editor fly camera; picking objects with the mouse
- [ ] Editor (Dear ImGui)
  - [x] Docked layout, scene tree and 3D viewport with shared selection
  - [ ] Inspector and the "Add node" dialog
  - [ ] Project manager and file browser
- [ ] Public API documentation

Later:

- [ ] Scripting
- [ ] Vulkan and Direct3D 12 backends, GPU selection in settings
- [ ] 2D

## Getting started

### Requirements

- CMake 3.24+
- A C++20 compiler: GCC 13+ (Linux, or MinGW-w64 on Windows) or Visual Studio 2022
- Ninja (not needed with Visual Studio)

Libraries are downloaded by CMake during configuration, see [Third-party](#third-party).

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

make            # build (debug)
make test       # build and run the tests
make run        # build and run the engine with its editor
make release    # build release; PRESET=release also works with test and run
```

The `Makefile` is a thin wrapper over CMake presets, which can be used directly:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/yqlora
```

On WSL, Mesa falls back to software rendering (`llvmpipe`) by default. To render on the GPU
through Direct3D 12, and to pick a specific adapter on a machine with two GPUs:

```bash
export GALLIUM_DRIVER=d3d12
export MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA   # part of the adapter name
```

### Windows

With Visual Studio 2022:

```powershell
cmake --preset vs2022
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
.\build\vs2022\bin\Debug\yqlora.exe
```

With MinGW-w64 and Ninja in `PATH`, the same presets as on Linux work:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
.\build\debug\bin\yqlora.exe
```

Visual Studio 2022 and VS Code (CMake Tools) pick up `CMakePresets.json` automatically.

On laptops with two GPUs, applications built on the engine ask the NVIDIA and AMD drivers
for the discrete GPU. The choice can be overridden in Windows graphics settings.

## Editor

`yqlora` is a single program: the engine together with its editor, like `godot` for Godot.
Panels can be dragged and docked; the layout is remembered between runs.
The UI language is switched in View > Language. Controls in the 3D viewport:

| Input | Action |
|---|---|
| Left click | Select an object (also selects it in the scene tree), click on empty space to deselect |
| Hold right button + mouse | Look around; the cursor wraps around the window edges |
| Hold right button + `W` `A` `S` `D` | Fly, `Q` / `E` down and up, `Shift` faster |
| Mouse wheel | Change the fly speed |

## Project structure

| Path | Contents |
|---|---|
| `engine/include/` | Public API for games: math (`Vec3`, `Quat`, ...), scene, reflection, input, logging, `tr()` |
| `engine/internal/` | Engine internals: window, platform, rendering, `Application`, settings |
| `editor/` | Editor: fly camera, picking, selection; the `yqlora` program itself in `editor/app/` |
| `backends/opengl/` | OpenGL renderer backend |
| `tests/` | Unit tests |
| `assets/locales/` | UI translations |
| `assets/fonts/` | UI font |
| `third_party/glad/` | Generated OpenGL loader |

## Third-party

| Component | License |
|---|---|
| [SDL3](https://github.com/libsdl-org/SDL) | zlib |
| [glm](https://github.com/g-truc/glm) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | MIT |
| [doctest](https://github.com/doctest/doctest) | MIT |
| [glad](https://github.com/Dav1dde/glad) (generated loader) | CC0-1.0 or WTFPL; Khronos parts Apache-2.0 |
| [Noto Sans](https://github.com/notofonts/latin-greek-cyrillic) font | [SIL OFL 1.1](assets/fonts/OFL.txt) |

## License

[MIT](LICENSE)
