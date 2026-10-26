# Yqlora Engine

[English version](README.md)

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows-lightgrey.svg)

3D игровой движок на C++20: дерево сцены в стиле Godot, компоненты на узлах
и рендерер, скрытый за интерфейсом, не зависящим от графического API.

> **Статус:** ранняя разработка. Ядро движка готово, следующий этап - редактор.

## Возможности

- **Дерево сцены** в духе Godot: `Node`, `Node3D`, `Camera3D`. У сцены один корень любого
  типа, сцена может быть пустой.
- **Компоненты на узлах.** У каждого `Node3D` есть встроенный `Transform3D`, который нельзя
  удалить; у простого `Node` трансформа нет, он нужен для логики и группировки.
- **Реестр типов со свойствами.** Узлы и компоненты создаются по имени типа и описывают свои
  редактируемые свойства. На этом строятся инспектор редактора и файлы сцен.
- **Файлы сцен** в JSON со стабильными ID узлов. Удалённые компоненты и свойства пропускаются
  с предупреждением, поэтому старые сцены продолжают открываться.
- **Граница публичного API.** Игровой код видит только `engine/include/`; окно, платформа
  и рендер лежат в `engine/internal/`, разделение проверяет CMake.
- **Сменный рендерер.** Сейчас OpenGL 3.3, в планах Vulkan и Direct3D 12.
- **Локализация** интерфейса движка (английский, русский).
- **Юнит-тесты** на doctest.

## Планы

- [x] Дерево сцены, компоненты, трансформы
- [x] Реестр типов и сохранение сцен
- [ ] Рендер: отрисовка в текстуру, шейдеры, меши, бесконечная сетка редактора
- [ ] Ввод: мышь и зажатые клавиши, свободная камера редактора
- [ ] Редактор (Dear ImGui): менеджер проектов, дерево сцены, инспектор, файлы, 3D-вид
- [ ] Скрипты
- [ ] 2D

## Сборка

### Что нужно

- CMake 3.24+
- Компилятор C++20: GCC 13+ или Visual Studio 2022
- Ninja (Linux)

SDL3, glm, nlohmann/json и doctest CMake скачивает сам при конфигурации.

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

make            # собрать debug
make test       # собрать и прогнать тесты
make run        # собрать и запустить sandbox
make release    # собрать release; PRESET=release работает и с test, и с run
```

`Makefile` - тонкая обёртка над пресетами CMake, их можно вызывать напрямую:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/sandbox
```

В WSL Mesa может выбрать программный рендер (`llvmpipe`).
Чтобы рисовать на видеокарте: `export GALLIUM_DRIVER=d3d12`.

### Windows (Visual Studio 2022)

```powershell
cmake --preset vs2022
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
.\build\vs2022\bin\Debug\sandbox.exe
```

Visual Studio 2022 и VS Code (CMake Tools) подхватывают `CMakePresets.json` сами.

### Sandbox

Небольшое приложение для ручной проверки движка, пока нет редактора.
`Esc` - выход, `F2` - переключить язык интерфейса.

## Структура

| Папка | Что внутри |
|---|---|
| `engine/include/` | Публичный API для игр: математика (`Vec3`, `Quat`, ...), сцена, реестр типов, ввод, лог, `tr()` |
| `engine/internal/` | Внутренности движка: окно, платформа, рендер, `Application`, настройки |
| `backends/opengl/` | Рендерер на OpenGL |
| `sandbox/` | Приложение для ручной проверки |
| `tests/` | Юнит-тесты |
| `assets/locales/` | Переводы интерфейса |
| `third_party/glad/` | Сгенерированный загрузчик OpenGL |

## Лицензия

[MIT](LICENSE)
