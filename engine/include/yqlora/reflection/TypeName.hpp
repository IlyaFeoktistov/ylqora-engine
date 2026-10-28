#pragma once

#include "yqlora/scene/LifecyclePhase.hpp"

#include <string_view>
#include <type_traits>

// Имя типа узла или компонента и его база в реестре. По имени тип ищется в
// реестре и пишется в файлы сцен. Ставится в открытой части каждого класса,
// который регистрируется:
//
//   class Camera3D : public Node3D {
//   public:
//       YQLORA_TYPE(Camera3D, Node3D)
//       ...
//
// Base - класс, от которого унаследован Class: Component для компонентов
// прямо от него. Без макроса наследник выдавал бы себя за базовый тип.
// Регистрация не компилируется, если макрос забыли или Base не база Class.
//
// Имя - функция, а не статическая константа: константу-данное из класса
// в dll MinGW подключить не может.
#define YQLORA_TYPE(Class, Base)                                                                                       \
    using YqloraThisType = Class;                                                                                      \
    using YqloraBaseType = Base;                                                                                       \
                                                                                                                       \
    static constexpr std::string_view staticTypeName()                                                                 \
    {                                                                                                                  \
        return #Class;                                                                                                 \
    }                                                                                                                  \
                                                                                                                       \
    std::string_view typeName() const override                                                                         \
    {                                                                                                                  \
        return staticTypeName();                                                                                       \
    }                                                                                                                  \
                                                                                                                       \
    /* Узнаёт, какие фазы кадра переопределены: снаружи защищённые методы не видны. */                                 \
    friend struct ::yqlora::PhaseDetector;

namespace yqlora {

// Имя типа T из YQLORA_TYPE; не компилируется, если в T макроса нет.
template <typename T>
constexpr std::string_view typeNameOf()
{
    static_assert(
        std::is_same_v<typename T::YqloraThisType, T>,
        "Add YQLORA_TYPE(ClassName, BaseName) to the public part of the class"
    );

    return T::staticTypeName();
}

} // namespace yqlora
