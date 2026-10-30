# Архитектура

[English version](README.md)

Контракты подсистем движка, по документу на подсистему. Каждый меняется
вместе с кодом, который описывает.

Пометки статуса: **[done]** сделано, **[next]** следующий этап работы (до
скриптов на C++), **[planned]** позже, с шагом, к которому относится.

| Документ | О чём |
|---|---|
| [Сцена](scene.ru.md) | узлы, компоненты, ресурсы, жизненный цикл, ссылки и сериализация |
| [Типы и рефлексия](reflection.ru.md) | реестр типов, свойства, методы, типы, объявленные во время выполнения |
| [Контекст движка и кадр](frame.ru.md) | службы движка, игровое время, системы сцены, порядок кадра |
| [Рендер](rendering.ru.md) | камеры, слои, виды, графика редактора |
| [Скрипты](scripting.ru.md) | C++, Luau, визуальные скрипты, горячая перезагрузка |
| [Редактор кода](code-editor.ru.md) | вкладка "Скрипты": Luau с подсветкой и подсказками, позже шейдеры |
| [Плагины](plugins.ru.md) | готовые скрипты, сцены и ресурсы из репозитория плагинов, менеджер плагинов |
| [Настройки проекта](project-settings.ru.md) | настройки игры, которые лежат в её проекте |

## Раскладка кода

Каждая подсистема - своя папка и сама регистрирует свои типы
(`registerSceneTypes`, `registerRenderTypes` в `internal/`);
`registerEngineTypes` в `core/` только вызывает их по порядку. Публичные
заголовки лежат в `include/`, заголовки только для движка и редактора - в
`internal/`, в `src/` только файлы `.cpp`.

```
engine/include/yqlora/core/        engine services, engine types, log, version
engine/include/yqlora/reflection/  TypeRegistry, Property, Method, component rules
engine/include/yqlora/scene/       Node, Node3D, Transform3D, Component, Scene, scene systems
engine/include/yqlora/render/      Camera3D, MeshRenderer, RenderSystem; lights        [step 5]
engine/include/yqlora/physics/     RigidBody, colliders, PhysicsWorld                  [step 6]
engine/include/yqlora/audio/       AudioSource, AudioListener, AudioWorld              [step 7]
engine/include/yqlora/ui/          Control, Label, Button                              [step 8]
```
