# yqlora-engine

2D/3D игровой движок на C++20. Рендерер подключается через интерфейс `yqlora::Renderer`;
сейчас реализован OpenGL 3.3, позже Vulkan / Direct3D 12.

## Структура

| Папка | Что внутри |
|---|---|
| `engine/include/` | публичный API для игр: математика (`Vec3`, `Quat`, ...), сцена (`Node`, `Node3D`, `Camera3D`, `Transform3D`, компоненты), ввод, лог, `tr()` |
| `engine/internal/` | внутренности движка: окно, платформа, рендер, `Application`, настройки. Игровому коду недоступны |
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

make            # собрать debug
make test       # собрать и прогнать тесты
make run        # собрать и запустить sandbox
make release    # собрать release (PRESET=release работает и с test/run)
```

Без make то же самое делается через пресеты CMake:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/sandbox
```

В WSL Mesa по умолчанию может выбрать программный рендер (`llvmpipe`).
Чтобы рисовать на видеокарте: `export GALLIUM_DRIVER=d3d12`.

### Windows (Visual Studio 2022)

```powershell
cmake --preset vs2022
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
.\build\vs2022\bin\Debug\sandbox.exe
```

Visual Studio 2022 и VS Code (расширение CMake Tools) подхватывают `CMakePresets.json` сами:
конфигурацию можно выбрать в интерфейсе и собирать без командной строки.

## Sandbox

Esc - выход, F2 - переключить язык интерфейса. Выбранный язык сохраняется в
`~/.local/share/Yqlora/Sandbox/settings.json` (Linux) или `%APPDATA%\Yqlora\Sandbox\settings.json` (Windows).
