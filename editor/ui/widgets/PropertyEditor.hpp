#pragma once

#include "yqlora/reflection/Property.hpp"

#include <string>

namespace yqlora {

class Component;
class Node;
struct EditorColors;

} // namespace yqlora

namespace yqlora::editor {

struct PropertyResult {
    // Свойство само не меняется: его записывают командой, чтобы правку можно было отменить.
    bool changed = false;
    PropertyValue value;
    // Значение тянут мышью: на это время стоит включить перенос курсора
    // у края окна, чтобы тянуть можно было бесконечно.
    bool dragging = false;
};

// Строка таблицы инспектора: подпись слева, значение справа. Вид поля
// выбирается по типу свойства и его подсказке.
//
// Числа меняются перетаскиванием подписи (у векторов - подписей X, Y, Z),
// а в само поле можно вписать значение. При перетаскивании Shift ускоряет
// в 10 раз, Ctrl замедляет в 10 раз.
PropertyResult drawProperty(const Property& property, Node& node, const EditorColors& colors);
PropertyResult drawProperty(const Property& property, Component& component, const EditorColors& colors);

// Подпись свойства на языке интерфейса: перевод ключа property.<имя>,
// а без перевода - имя с заглавной буквы.
std::string propertyLabel(const std::string& name);

} // namespace yqlora::editor
