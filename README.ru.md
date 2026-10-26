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
- **Графика редактора:** бесконечная сетка, которая гаснет с расстоянием, и тонкий контур
  вокруг выбранного объекта.
- **Камера редактора и выбор объектов.** Свободная камера живёт вне сцены, объекты
  выбираются кликом с точностью до треугольника.
- **Локализация** интерфейса движка (английский, русский).
- **Юнит-тесты** на doctest.

## Планы

MVP: минимальный редактор в духе Godot.

- [x] Дерево сцены, компоненты, трансформы
- [x] Реестр типов и сохранение сцен
- [x] Основа рендера: шейдеры, меши-примитивы, бесконечная сетка редактора, контур выбранного объекта
- [ ] Отрисовка в текстуру для окна сцены в редакторе
- [x] Ввод: клавиатура, мышь, колесо; свободная камера редактора; выбор объектов мышью
- [ ] Редактор (Dear ImGui)
  - [x] Панели с раскладкой, дерево сцены и 3D-вид с общим выделением
  - [ ] Инспектор и окно "Добавить узел"
  - [ ] Менеджер проектов и список файлов
- [ ] Документация публичного API

Потом:

- [ ] Скрипты
- [ ] Бэкенды Vulkan и Direct3D 12, выбор видеокарты в настройках
- [ ] 2D

## Сборка

### Что нужно

- CMake 3.24+
- Компилятор C++20: GCC 13+ (Linux или MinGW-w64 на Windows) или Visual Studio 2022
- Ninja (с Visual Studio не нужен)

Библиотеки CMake скачивает сам при конфигурации, список в разделе [Сторонние компоненты](#сторонние-компоненты).

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

make            # собрать debug
make test       # собрать и прогнать тесты
make run        # собрать и запустить движок с редактором
make release    # собрать release; PRESET=release работает и с test, и с run
```

`Makefile` - тонкая обёртка над пресетами CMake, их можно вызывать напрямую:

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/bin/yqlora
```

В WSL Mesa по умолчанию рисует программно (`llvmpipe`). Чтобы рисовать на видеокарте
через Direct3D 12 и выбрать конкретную, если их две:

```bash
export GALLIUM_DRIVER=d3d12
export MESA_D3D12_DEFAULT_ADAPTER_NAME=NVIDIA   # часть названия видеокарты
```

### Windows

С Visual Studio 2022:

```powershell
cmake --preset vs2022
cmake --build --preset vs2022-debug
ctest --preset vs2022-debug
.\build\vs2022\bin\Debug\yqlora.exe
```

С MinGW-w64 и Ninja в `PATH` работают те же пресеты, что на Linux:

```powershell
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
.\build\debug\bin\yqlora.exe
```

Visual Studio 2022 и VS Code (CMake Tools) подхватывают `CMakePresets.json` сами.

На ноутбуках с двумя видеокартами приложения на движке просят драйверы NVIDIA и AMD
запускать их на дискретной. Выбор можно поменять в настройках графики Windows.

## Редактор

`yqlora` - одна программа: движок вместе с редактором, как `godot` у Godot.
Панели можно перетаскивать и пристыковывать, раскладка запоминается между запусками.
Язык интерфейса переключается в меню Вид > Язык. Управление в 3D-виде:

| Ввод | Действие |
|---|---|
| Левый клик | Выбрать объект (он выделится и в дереве сцены), клик в пустоту снимает выбор |
| Зажать правую кнопку + мышь | Осмотреться; у края окна курсор переходит на противоположную сторону |
| Зажать правую кнопку + `W` `A` `S` `D` | Лететь, `Q` / `E` вниз и вверх, `Shift` быстрее |
| Колесо мыши | Скорость полёта |

## Структура

| Папка | Что внутри |
|---|---|
| `engine/include/` | Публичный API для игр: математика (`Vec3`, `Quat`, ...), сцена, реестр типов, ввод, лог, `tr()` |
| `engine/internal/` | Внутренности движка: окно, платформа, рендер, `Application`, настройки |
| `editor/` | Редактор: свободная камера, выбор объектов, выделение; сама программа `yqlora` в `editor/app/` |
| `backends/opengl/` | Рендерер на OpenGL |
| `tests/` | Юнит-тесты |
| `assets/locales/` | Переводы интерфейса |
| `assets/fonts/` | Шрифт интерфейса |
| `third_party/glad/` | Сгенерированный загрузчик OpenGL |

## Сторонние компоненты

| Компонент | Лицензия |
|---|---|
| [SDL3](https://github.com/libsdl-org/SDL) | zlib |
| [glm](https://github.com/g-truc/glm) | MIT |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT |
| [Dear ImGui](https://github.com/ocornut/imgui) | MIT |
| [doctest](https://github.com/doctest/doctest) | MIT |
| [glad](https://github.com/Dav1dde/glad) (сгенерированный загрузчик) | CC0-1.0 или WTFPL; части Khronos - Apache-2.0 |
| Шрифт [Noto Sans](https://github.com/notofonts/latin-greek-cyrillic) | [SIL OFL 1.1](assets/fonts/OFL.txt) |

## Лицензия

[MIT](LICENSE)
