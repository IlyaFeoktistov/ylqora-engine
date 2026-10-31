#include "yqlora/core/Log.hpp"

#include "yqlora/core/LogSettings.hpp"
#include "yqlora/core/LogSubscription.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <fstream>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace yqlora::log {

// Свой мьютекс у каждого слушателя: его вызывают вне общей блокировки, а
// отписка ждёт, пока идущий вызов закончится.
struct Subscriber {
    std::mutex mutex;
    Subscription::Listener listener;
    bool active = true;
};

namespace {

// Лог могут писать и из других потоков: например, ответ системного диалога.
std::mutex g_mutex;
std::ofstream g_file;
std::atomic<Level> g_minimumLevel = Level::Warning;
std::vector<std::shared_ptr<Subscriber>> g_subscribers;

const char* prefix(Level level)
{
    switch (level) {
    case Level::Info: return "INFO";
    case Level::Warning: return "WARN";
    case Level::Error: return "ERROR";
    }

    return "INFO";
}

} // namespace

bool isEnabled(Level level)
{
    return level >= g_minimumLevel.load(std::memory_order_relaxed);
}

void setMinimumLevel(Level level)
{
    g_minimumLevel.store(level, std::memory_order_relaxed);
}

void openFile(const std::filesystem::path& file)
{
    const std::lock_guard lock(g_mutex);
    g_file.open(file, std::ios::out | std::ios::trunc);
}

void write(Level level, std::string_view message)
{
    std::vector<std::shared_ptr<Subscriber>> subscribers;
    {
        const std::lock_guard lock(g_mutex);
        std::fprintf(stderr, "[yqlora][%s] %.*s\n", prefix(level), static_cast<int>(message.size()), message.data());
        if (g_file.is_open()) {
            g_file << '[' << prefix(level) << "] " << message << std::endl;
        }
        subscribers = g_subscribers;
    }
    for (const auto& subscriber : subscribers) {
        const std::lock_guard lock(subscriber->mutex);
        if (subscriber->active) {
            subscriber->listener(level, message);
        }
    }
}

Subscription subscribe(Subscription::Listener listener)
{
    auto subscriber = std::make_shared<Subscriber>();
    subscriber->listener = std::move(listener);
    const std::lock_guard lock(g_mutex);
    g_subscribers.push_back(subscriber);

    return Subscription(std::move(subscriber));
}

Subscription::Subscription(std::shared_ptr<Subscriber> subscriber)
    : m_subscriber(std::move(subscriber))
{
}

Subscription::~Subscription()
{
    reset();
}

Subscription& Subscription::operator=(Subscription&& other) noexcept
{
    if (this != &other) {
        reset();
        m_subscriber = std::move(other.m_subscriber);
    }

    return *this;
}

void Subscription::reset()
{
    if (!m_subscriber) {
        return;
    }
    // Объявлен раньше блокировки: слушатель уничтожается уже после неё.
    const std::shared_ptr<Subscriber> subscriber = std::move(m_subscriber);
    {
        const std::lock_guard lock(g_mutex);
        std::erase(g_subscribers, subscriber);
    }
    // Вызов, который уже взял копию списка, держит мьютекс слушателя: ждём его.
    const std::lock_guard lock(subscriber->mutex);
    subscriber->active = false;
}

} // namespace yqlora::log
