# yqlora-engine

2D/3D игровой движок на C++20. Рендерер подключается через интерфейс `yqlora::Renderer`;
сейчас реализован OpenGL 3.3, позже — Vulkan / Direct3D 12.

## Структура

| Папка | Что внутри |
|---|---|
| `engine/` | ядро: сцена (`Node`, `Node3D`, `Camera3D`), окно, настройки, локализация |
| `backends/opengl/` | реализация рендерера на OpenGL |
| `sandbox/` | тестовое приложение для ручной проверки движка |
| `tests/` | юнит-тесты (doctest) |
| `assets/locales/` | переводы интерфейса (`en.json`, `ru.json`) |
| `third_party/glad/` | сгенерированный загрузчик OpenGL |

Остальные зависимости (SDL3, glm, nlohmann/json, doctest) скачиваются CMake при конфигурации.

## Сборка

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/bin/yqlora_tests
./build/bin/sandbox
```

В WSL Mesa по умолчанию может выбрать программный рендер (`llvmpipe`).
Чтобы рисовать на видеокарте: `export GALLIUM_DRIVER=d3d12`.

### Windows (Visual Studio 2022)

```powershell
cmake -S . -B build-win
cmake --build build-win --config Debug
.\build-win\bin\Debug\yqlora_tests.exe
.\build-win\bin\Debug\sandbox.exe
```

## Sandbox

Esc — выход, F2 — переключить язык интерфейса. Выбранный язык сохраняется в
`~/.local/share/Yqlora/Sandbox/settings.json` (Linux) или `%APPDATA%\Yqlora\Sandbox\settings.json` (Windows).
