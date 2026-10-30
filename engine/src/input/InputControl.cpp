#include "yqlora/input/InputControl.hpp"

#include "yqlora/input/Input.hpp"

#include <algorithm>

namespace yqlora {

namespace {

template <typename Enum, std::size_t Count>
std::optional<Enum> findName(const std::array<std::string_view, Count>& names, std::string_view name)
{
    const auto it = std::ranges::find(names, name);

    return it != names.end() ? std::optional(static_cast<Enum>(it - names.begin())) : std::nullopt;
}

} // namespace

std::optional<InputControl> controlFromName(std::string_view name)
{
    if (const auto key = findName<Key>(kKeyNames, name); key && *key != Key::Unknown) {
        return *key;
    }
    if (const auto button = findName<MouseButton>(kMouseButtonNames, name)) {
        return *button;
    }

    return std::nullopt;
}

std::string_view controlName(const InputControl& control)
{
    if (const Key* key = std::get_if<Key>(&control)) {
        return kKeyNames[static_cast<std::size_t>(*key)];
    }

    return kMouseButtonNames[static_cast<std::size_t>(std::get<MouseButton>(control))];
}

bool isControlDown(const Input& input, const InputControl& control)
{
    if (const Key* key = std::get_if<Key>(&control)) {
        return input.isKeyDown(*key);
    }

    return input.isMouseButtonDown(std::get<MouseButton>(control));
}

bool wasControlPressed(const Input& input, const InputControl& control)
{
    if (const Key* key = std::get_if<Key>(&control)) {
        return input.wasKeyPressed(*key);
    }

    return input.wasMouseButtonPressed(std::get<MouseButton>(control));
}

} // namespace yqlora
