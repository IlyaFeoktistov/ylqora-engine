# Yqlora Engine

[Русская версия](README.ru.md)

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows-lightgrey.svg)

A 3D game engine with an editor, written in C++20.

> **Status:** early development.

## Roadmap

- [x] Scene tree, components, transforms
- [x] Type registry and scene files
- [x] Renderer basics: meshes, editor grid, selection outline
- [x] Input, editor camera, picking objects with the mouse
- [ ] Editor: project manager, scene tree, inspector, file browser
- [ ] Documentation: public API, engine internals, editor user guide

## Building

Requirements: CMake 3.24+, a C++20 compiler (GCC 13+, MinGW-w64 or Visual Studio 2022), Ninja.
Libraries are downloaded by CMake during configuration.

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

make run    # build and run
make test   # build and run the tests
```

### Windows

```powershell
cmake --preset debug          # MinGW-w64 + Ninja; with Visual Studio use the vs2022 preset
cmake --build --preset debug
.\build\debug\bin\yqlora.exe
```

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
