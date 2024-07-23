#include "time_wheel_scheduler.h"
#include "Rate.hpp"

static uint32_t s_inc_id = 1;

TimeWheelScheduler::TimeWheelScheduler(uint32_t timer_step_ms)
    : timer_step_ms_(timer_step_ms)
      , stop_(false) {
}

bool TimeWheelScheduler::start() {
    if (timer_step_ms_ < 50 || time_wheels_.empty()) {
        return false;
    }

    thread_ = std::thread([this] { run(); });
    return true;
}

void TimeWheelScheduler::run() {
    Rate rate(1000.0 / timer_step_ms_);
    while (true) {
        //std::this_thread::sleep_for(std::chrono::milliseconds(timer_step_ms_));
        rate.sleep();
        std::lock_guard<std::mutex> lock(mutex_);
        if (stop_) {
            break;
        }

        TimeWheelPtr least_time_wheel = get_least_time_wheel();
        least_time_wheel->increase();
        std::list<TimerPtr> slot = std::move(least_time_wheel->get_and_clear_current_slot());
        for (const TimerPtr &timer: slot) {
            if (auto it = cancel_timer_ids_.find(timer->id()); it != cancel_timer_ids_.end()) {
                cancel_timer_ids_.erase(it);
                continue;
            }

            timer->run();
            if (timer->repeated()) {
                timer->update_when_time();
                get_greatest_time_wheel()->add_timer(timer);
            }
        }
    }
}

void TimeWheelScheduler::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }

    if (thread_.joinable()) {
        thread_.join();
    }
}

TimeWheelPtr TimeWheelScheduler::get_greatest_time_wheel() {
    return time_wheels_.empty() ? TimeWheelPtr{} : time_wheels_.front();
}

TimeWheelPtr TimeWheelScheduler::get_least_time_wheel() {
    return time_wheels_.empty() ? TimeWheelPtr() : time_wheels_.back();
}

void TimeWheelScheduler::append_time_wheel(uint32_t scales, uint32_t scale_unit_ms, const std::string &name) {
    auto time_wheel = std::make_shared<TimeWheel>(scales, scale_unit_ms, name);
    if (!time_wheels_.empty()) {
        TimeWheelPtr greater_time_wheel = time_wheels_.back();
        greater_time_wheel->set_less_level_tw(time_wheel.get());
        time_wheel->set_greater_level_tw(greater_time_wheel.get());
    }
    time_wheels_.push_back(time_wheel);
}

uint32_t TimeWheelScheduler::create_timer_at(int64_t when_ms, const TimerTask &task) {
    if (time_wheels_.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ++s_inc_id;
    get_greatest_time_wheel()->add_timer(std::make_shared<Timer>(s_inc_id, when_ms, 0, task));

    return s_inc_id;
}

uint32_t TimeWheelScheduler::create_timer_after(int64_t delay_ms, const TimerTask &task) {
    int64_t when = get_now_timestamp() + delay_ms;
    return create_timer_at(when, task);
}

uint32_t TimeWheelScheduler::create_timer_every(int64_t interval_ms, const TimerTask &task) {
    if (time_wheels_.empty()) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    ++s_inc_id;
    int64_t when = get_now_timestamp() + interval_ms;
    get_greatest_time_wheel()->add_timer(std::make_shared<Timer>(s_inc_id, when, interval_ms, task));

    return s_inc_id;
}

void TimeWheelScheduler::cancel_timer(uint32_t timer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    cancel_timer_ids_.insert(timer_id);
}
