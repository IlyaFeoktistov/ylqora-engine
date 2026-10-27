#pragma once

#include <string_view>

// Имя типа узла или компонента: по нему тип ищется в реестре и пишется в
// файлы сцен. Ставится в открытой части каждого класса, который регистрируется:
//
//   class Camera3D : public Node3D {
//   public:
//       YQLORA_TYPE(Camera3D)
//       ...
//
// Без макроса наследник выдавал бы себя за базовый тип. Регистрация это
// проверяет и не компилируется, если макрос забыли.
//
// Имя - функция, а не статическая константа: константу-данное из класса
// в dll MinGW подключить не может.
#define YQLORA_TYPE(Class)                                                                                             \
    using YqloraThisType = Class;                                                                                      \
                                                                                                                       \
    static constexpr std::string_view staticTypeName()                                                                 \
    {                                                                                                                  \
        return #Class;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    std::string_view typeName() const override                                                                         \
    {                                                                                                                  \
        return staticTypeName();                                                                                       \
    }
