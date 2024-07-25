#pragma once

#include <cstdint>
#include <functional>
#include <memory>

using TimerTask = std::function<void()>;

class TimerTaskEntry {
public:
    TimerTaskEntry(uint32_t id, uint64_t when_ms, uint32_t interval_ms, TimerTask handler)
        : id_{id}, when_ms_{when_ms}, interval_ms_{interval_ms}, repeated_{interval_ms > 0}, task_{std::move(handler)} {
    }

    void run() {
        if (task_) {
            task_();
        }
    }

    [[nodiscard]] uint32_t id() const {
        return id_;
    }

    [[nodiscard]] uint64_t when_ms() const {
        return when_ms_;
    }

    [[nodiscard]] bool repeated() const {
        return repeated_;
    }

    void update_when_time() {
        when_ms_ += interval_ms_;
    }

private:
    const uint32_t id_;
    uint64_t when_ms_;
    const uint32_t interval_ms_;
    const bool repeated_;
    TimerTask task_;
};

using TimerTaskEntryPtr = std::shared_ptr<TimerTaskEntry>;
