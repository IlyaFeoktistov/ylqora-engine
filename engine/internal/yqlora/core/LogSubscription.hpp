#pragma once

#include "yqlora/core/Export.hpp"
#include "yqlora/core/Log.hpp"

#include <functional>
#include <memory>
#include <string_view>

namespace yqlora::log {

struct Subscriber;

// Подписка на сообщения лога: кроме консоли и файла, их получает слушатель.
// Слушателя вызывают в потоке, который пишет в лог, вне общей блокировки лога,
// поэтому он должен быть быстрым и потокобезопасным: например, положить
// запись в очередь. Сам слушатель не пишет в лог и не отменяет свою подписку.
// После уничтожения подписки слушателя больше не вызывают.
class YQLORA_API Subscription {
public:
    using Listener = std::function<void(Level level, std::string_view message)>;

    Subscription() = default;
    ~Subscription();

    Subscription(Subscription&& other) noexcept = default;
    Subscription& operator=(Subscription&& other) noexcept;

    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;

private:
    friend YQLORA_API Subscription subscribe(Listener listener);

    explicit Subscription(std::shared_ptr<Subscriber> subscriber);

    void reset();

    std::shared_ptr<Subscriber> m_subscriber;
};

YQLORA_API Subscription subscribe(Subscription::Listener listener);

} // namespace yqlora::log
