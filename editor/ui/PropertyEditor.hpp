#pragma once

#include <string>

namespace yqlora {

class Component;
class Node;
class Property;

// Поле для одного свойства в таблице инспектора: подпись слева, значение справа.
// Вид поля выбирается по типу свойства и его подсказке. Возвращает true,
// если значение поменяли.
bool drawProperty(const Property& property, Node& node);
bool drawProperty(const Property& property, Component& component);

// Подпись свойства на языке интерфейса: перевод ключа property.<имя>,
// а без перевода - имя с заглавной буквы.
std::string propertyLabel(const std::string& name);

} // namespace yqlora
