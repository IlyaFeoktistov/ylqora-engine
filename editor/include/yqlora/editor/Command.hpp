#pragma once

namespace yqlora {

class EditorSession;

// Одна правка сцены, которую можно отменить и сделать снова. Узлы команды
// ищут по ID, а не по указателю: после Stop сцена пересоздаётся из файла,
// и прежние указатели уже никуда не ведут.
class Command {
public:
    virtual ~Command() = default;

    // Вызывается и при повторе после отмены. false, если делать нечего
    // (например, узла уже нет): тогда команда не попадает в историю.
    virtual bool apply(EditorSession& session) = 0;
    virtual void revert(EditorSession& session) = 0;

    // Вбирает следующую команду той же правки, чтобы одна отмена откатила
    // всё перетаскивание. next уже сделана. true, если вобрала.
    virtual bool merge(const Command& /*next*/)
    {
        return false;
    }
};

} // namespace yqlora
