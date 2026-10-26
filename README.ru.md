# Yqlora Engine

[English version](README.md)

![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)
![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20Windows-lightgrey.svg)

3D игровой движок с редактором на C++20.

> **Статус:** ранняя разработка.

## Планы

- [x] Дерево сцены, компоненты, трансформы
- [x] Реестр типов и файлы сцен
- [x] Основа рендера: меши, сетка редактора, контур выбранного объекта
- [x] Ввод, камера редактора, выбор объектов мышью
- [ ] Редактор: менеджер проектов, дерево сцены, инспектор, список файлов
- [ ] Документация: публичный API, внутреннее устройство движка, руководство по редактору

## Сборка

Нужны CMake 3.24+, компилятор C++20 (GCC 13+, MinGW-w64 или Visual Studio 2022) и Ninja.
Библиотеки CMake скачивает сам при конфигурации.

### Linux / WSL

```bash
sudo apt install build-essential cmake ninja-build libx11-dev libxext-dev libxrandr-dev \
  libxcursor-dev libxi-dev libxss-dev libxfixes-dev libxtst-dev libwayland-dev \
  libxkbcommon-dev libegl-dev libdecor-0-dev

make run    # собрать и запустить
make test   # собрать и прогнать тесты
```

### Windows

```powershell
cmake --preset debug          # MinGW-w64 + Ninja; с Visual Studio пресет vs2022
cmake --build --preset debug
.\build\debug\bin\yqlora.exe
```

Программа запускается без консоли и пишет в лог только предупреждения и ошибки;
`yqlora.exe --debug` пишет всё и показывает лог в консоли.

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
