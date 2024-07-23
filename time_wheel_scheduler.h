#ifndef TIME_WHEEL_SCHEDULER_H_
#define TIME_WHEEL_SCHEDULER_H_

#include "time_wheel.h"
#include <mutex>
#include <vector>
#include <thread>
#include <unordered_set>

class TimeWheel;

class TimeWheelScheduler {
public:
    explicit TimeWheelScheduler(uint32_t timer_step_ms = 50);

    // Return timer id. Return 0 if the timer creation fails.
    uint32_t create_timer_at(int64_t when_ms, const TimerTask &task);

    uint32_t create_timer_after(int64_t delay_ms, const TimerTask &task);

    uint32_t create_timer_every(int64_t interval_ms, const TimerTask &task);

    void cancel_timer(uint32_t timer_id);

    bool start();

    void stop();

    void append_time_wheel(uint32_t scales, uint32_t scale_unit_ms, const std::string &name = "");

private:
    void run();

    TimeWheelPtr get_greatest_time_wheel();

    TimeWheelPtr get_least_time_wheel();

private:
    std::mutex mutex_;
    std::thread thread_;

    const uint32_t timer_step_ms_;
    bool stop_;

    std::unordered_set<uint32_t> cancel_timer_ids_;

    std::vector<TimeWheelPtr> time_wheels_;
};

#endif  // TIME_WHEEL_SCHEDULER_H_
