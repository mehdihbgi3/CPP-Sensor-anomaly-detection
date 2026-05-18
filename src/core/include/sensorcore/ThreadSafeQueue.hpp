#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>
#include <stdexcept>

namespace sensorcore {

template <typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        condVar_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        condVar_.wait(lock, [this] { return !queue_.empty() || stopped_; });

        if (stopped_ && queue_.empty()) {
            throw std::runtime_error("Queue stopped");
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    std::optional<T> popWithTimeout(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!condVar_.wait_for(lock, timeout, [this] { return !queue_.empty() || stopped_; })) {
            return std::nullopt;
        }

        if (stopped_ && queue_.empty()) {
            return std::nullopt;
        }

        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    [[nodiscard]] bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }

    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::queue<T> empty;
        std::swap(queue_, empty);
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        condVar_.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(mutex_);
        stopped_ = false;
    }

    [[nodiscard]] bool isStopped() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stopped_;
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable condVar_;
    std::queue<T> queue_;
    bool stopped_ = false;
};

}  