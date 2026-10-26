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
- **Localization** of the engine UI (English, Russian).
- **Unit tests** with doctest.

## Roadmap

- [x] Scene tree, components, transforms
- [x] Type registry and scene serialization
- [x] Renderer basics: shaders, primitive meshes, infinite editor grid, selection outline
- [ ] Offscreen render targets for the editor viewport
- [ ] Input: mouse and held keys, editor fly camera
- [ ] Editor (Dear ImGui): project manager, scene tree, inspector, file browser, 3D viewport
- [ ] Scripting
- [ ] 2D

## Getting started

### Requirements

- CMake 3.24+
- A C++20 compiler: GCC 13+ or Visual Studio 2022
- Ninja (Linux)

SDL3, glm, nlohmann/json and doctest are downloaded by CMake during configuration.

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

make            # build (debug)
make test       # build and run the tests
make run        # build and run the sandbox
make release    # build release; PRESET=release also works with test and run
```

The `Makefile` is a thin wrapper over CMake presets, which can be used directly:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/sandbox
```

On WSL, Mesa falls back to software rendering (`llvmpipe`) by default. To render on the GPU
through Direct3D 12, and to pick a specific adapter on a machine with two GPUs:

```bash
export GALLIUM_DRIVER=d3d12
export MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA   # part of the adapter name
```

### Windows (Visual Studio 2022)

```powershell
cmake --preset vs2022
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
.\build\vs2022\bin\Debug\sandbox.exe
```

Visual Studio 2022 and VS Code (CMake Tools) pick up `CMakePresets.json` automatically.

On laptops with two GPUs, applications built on the engine ask the NVIDIA and AMD drivers
for the discrete GPU. The choice can be overridden in Windows graphics settings.

### Sandbox

A small app for trying the engine by hand until the editor arrives.
`Esc` quits, `Tab` selects the next object, `F2` switches the UI language.

## Project structure

| Path | Contents |
|---|---|
| `engine/include/` | Public API for games: math (`Vec3`, `Quat`, ...), scene, reflection, input, logging, `tr()` |
| `engine/internal/` | Engine internals: window, platform, rendering, `Application`, settings |
| `backends/opengl/` | OpenGL renderer backend |
| `sandbox/` | Manual testing app |
| `tests/` | Unit tests |
| `assets/locales/` | UI translations |
| `third_party/glad/` | Generated OpenGL loader |

## License

[MIT](LICENSE)
