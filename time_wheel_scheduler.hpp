#pragma once

#include "time_wheel.hpp"
#include <mutex>
#include <vector>
#include <thread>
#include <unordered_set>

class TimeWheel;

class TimeWheelScheduler {
public:
    explicit TimeWheelScheduler(uint32_t timer_step_ms = 50);

    void append_time_wheel(uint32_t scales, uint32_t scale_unit_ms, const std::string &name = "");
    bool start();
    void stop();

    // Return timer id. Return 0 if the timer creation fails.
    uint32_t create_timer_at(uint64_t when_ms, TimerTask task);
    uint32_t create_timer_after(uint32_t delay_ms, TimerTask task);
    uint32_t create_timer_every(uint32_t interval_ms, TimerTask task);
    void cancel_timer(uint32_t timer_id);

private:
    void run();
    [[nodiscard]] TimeWheelPtr greatest_time_wheel() const;
    [[nodiscard]] TimeWheelPtr least_time_wheel() const;
    uint32_t create_timer(uint64_t when_ms, uint32_t interval_ms, TimerTask task);

private:
    std::mutex mutex_;
    std::thread thread_;

    const uint32_t timer_step_ms_;
    bool stop_;

    std::unordered_set<uint32_t> cancel_timer_ids_;
    std::vector<TimeWheelPtr> time_wheels_;
};
