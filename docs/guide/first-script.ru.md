# Первый скрипт

[English version](first-script.md)

Скрипт - это C++-класс, который вешается на узел как компонент. Скрипты игры
живут в одной разделяемой библиотеке, модуле игры: `game.dll` на Windows,
`libgame.so` на Linux. Редактор загружает её из папки `.build/` проекта,
когда проект открывается.

До шага 3 (сборка скриптов из редактора) модуль игры собирается внутри
CMake-дерева движка, рядом с примерами. В этом руководстве получится скрипт
`Pulse`, который заставляет узел увеличиваться и уменьшаться.

## 1. Написать скрипт

Создайте `examples/my-game/Pulse.cpp`:

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

- `YQLORA_SCRIPT(Pulse)` задаёт имя типа; это имя хранят файлы сцен.
- `YQLORA_FIELDS` перечисляет поля, которые показывает инспектор и
  сохраняют файлы сцен, с их атрибутами. Неперечисленные поля (здесь
  `m_baseScale`, `m_elapsed`) живут только во время игры.
- `YQLORA_MODULE_ENTRY` - единственное место, где библиотека регистрирует
  свои скрипты. Входная функция у модуля ровно одна.
- Вызовы жизненного цикла (`onStart`, `onUpdate`, ...) и их порядок описаны
  в [скриптах](../architecture/scripting.ru.md#контракт-жизненного-цикла).

## 2. Собрать библиотеку

Создайте `examples/my-game/CMakeLists.txt`:

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

Добавьте `add_subdirectory(my-game)` в `examples/CMakeLists.txt` и соберите:

```bash
cmake --preset debug
cmake --build --preset debug
```

Библиотека появится в `build/debug/examples/my-game/`.

## 3. Положить её в проект

Скопируйте `game.dll` (`libgame.so`) в папку `.build/` проекта, рядом с
`project.yqlora`. Панель "Файлы" эту папку не показывает, ресурсом она не
бывает.

## 4. Использовать в редакторе

Откройте проект. Выберите узел, нажмите "Добавить компонент": `Pulse` в
группе "Скрипты". Поменяйте `amplitude` и `frequency` в инспекторе, нажмите
Play.

Чтобы поменять код: закройте проект (на Windows загруженную библиотеку не
заменить), соберите, снова скопируйте библиотеку, откройте проект. Горячая
перезагрузка появится в шаге 3.

Если проект открыт без библиотеки, скрипты в его сценах хранятся
заглушками: инспектор показывает их как не найденные, сохранение пишет их
данные обратно без изменений, и они возвращаются, когда библиотека снова на
месте.

## Ограничения

- Скрипты - нативный код в процессе редактора. Исключение из вызова
  жизненного цикла выключает компонент и уходит в лог, но падение (нулевой
  указатель) роняет редактор вместе со скриптом, а бесконечный цикл его
  вешает. Сохраняйтесь перед Play.
- Один модуль игры на проект. Скрипт с уже занятым именем (типом движка или
  другим скриптом) не регистрируется; в логе написано, каким модулем занято имя.
- Имя поля в `field("name", ...)` хранится в файлах сцен. Переименовав его,
  отметьте старое: `.formerly("oldName")`; переименовав класс -
  `registerScript<T>().formerly("OldName")`.
