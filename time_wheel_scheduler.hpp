#pragma once

#include "time_wheel.hpp"
#include "Rate.hpp"
#include <mutex>
#include <vector>
#include <thread>
#include <unordered_set>

class TimeWheel;

class TimeWheelScheduler {
public:
    explicit TimeWheelScheduler(uint32_t timer_step_ms = 50): timer_step_ms_{timer_step_ms}, stop_{false} {
    }

    void append_time_wheel(uint32_t scales, uint32_t scale_unit_ms, const std::string &name = "") {
        auto time_wheel = std::make_shared<TimeWheel>(scales, scale_unit_ms, name);
        if (!time_wheels_.empty()) {
            TimeWheelPtr greater_time_wheel = time_wheels_.back();
            greater_time_wheel->set_less_level_tw(time_wheel.get());
            time_wheel->set_greater_level_tw(greater_time_wheel.get());
        }
        time_wheels_.push_back(std::move(time_wheel));
    }

    bool start() {
        if (timer_step_ms_ < 50 || time_wheels_.empty()) {
            return false;
        }

        thread_ = std::thread([this] { run(); });
        return true;
    }

    void stop() {
        stop_.store(true, std::memory_order_release);
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    // Return timer id. Return 0 if the timer creation fails.
    uint32_t create_timer_at(uint64_t when_ms, TimerTask task){
        return create_timer(when_ms, 0, std::move(task));
    }

    uint32_t create_timer_after(uint32_t delay_ms, TimerTask task) {
        const uint64_t when_ms = get_now_timestamp() + delay_ms;
        return create_timer(when_ms, 0, std::move(task));
    }

    uint32_t create_timer_every(uint32_t interval_ms, TimerTask task){
        const uint64_t when_ms = get_now_timestamp() + interval_ms;
        return create_timer(when_ms, interval_ms, std::move(task));
    }

    void cancel_timer(uint32_t timer_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        cancel_timer_ids_.insert(timer_id);
    }

private:
    void run() {
        Rate rate(1000.0 / timer_step_ms_);
        while (!stop_.load(std::memory_order_acquire)) {
            rate.sleep();
            std::lock_guard<std::mutex> lock(mutex_);
            TimeWheelPtr time_wheel = least_time_wheel();
            std::list<TimerTaskEntryPtr> slot = time_wheel->take_current_slot();
            time_wheel->increase();
            for (TimerTaskEntryPtr& timer: slot) {
                auto iter = cancel_timer_ids_.find(timer->id());
                if (iter != cancel_timer_ids_.end()) {
                    cancel_timer_ids_.erase(iter);
                    continue;
                }

                timer->run();
                if (timer->repeated()) {
                    timer->update_when_time();
                    greatest_time_wheel()->add_timer(timer);
                }
            }
        }
    }

    [[nodiscard]] TimeWheelPtr greatest_time_wheel() const {
        return time_wheels_.empty() ? nullptr : time_wheels_.front();
    }

    [[nodiscard]] TimeWheelPtr least_time_wheel() const {
        return time_wheels_.empty() ? nullptr : time_wheels_.back();
    }

    uint32_t create_timer(uint64_t when_ms, uint32_t interval_ms, TimerTask task) {
        if (time_wheels_.empty()) {
            return 0;
        }

        static uint32_t global_inc_id{};
        std::lock_guard<std::mutex> lock(mutex_);
        ++global_inc_id;
        greatest_time_wheel()->add_timer(std::make_shared<TimerTaskEntry>(global_inc_id, when_ms, interval_ms, std::move(task)));
        return global_inc_id;
    }

private:
    const uint32_t timer_step_ms_;
    std::atomic_bool stop_;
    std::thread thread_;
    std::mutex mutex_;
    std::unordered_set<uint32_t> cancel_timer_ids_;
    std::vector<TimeWheelPtr> time_wheels_;
};
