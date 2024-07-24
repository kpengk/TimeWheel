#include "time_wheel_scheduler.hpp"
#include "Rate.hpp"

TimeWheelScheduler::TimeWheelScheduler(uint32_t timer_step_ms)
    : timer_step_ms_{timer_step_ms}, stop_{false} {
}

void TimeWheelScheduler::append_time_wheel(uint32_t scales, uint32_t scale_unit_ms, const std::string &name) {
    auto time_wheel = std::make_shared<TimeWheel>(scales, scale_unit_ms, name);
    if (!time_wheels_.empty()) {
        TimeWheelPtr greater_time_wheel = time_wheels_.back();
        greater_time_wheel->set_less_level_tw(time_wheel.get());
        time_wheel->set_greater_level_tw(greater_time_wheel.get());
    }
    time_wheels_.push_back(std::move(time_wheel));
}

bool TimeWheelScheduler::start() {
    if (timer_step_ms_ < 50 || time_wheels_.empty()) {
        return false;
    }

    thread_ = std::thread([this] { run(); });
    return true;
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

TimeWheelPtr TimeWheelScheduler::greatest_time_wheel() const{
    return time_wheels_.empty() ? nullptr : time_wheels_.front();
}

TimeWheelPtr TimeWheelScheduler::least_time_wheel() const {
    return time_wheels_.empty() ? nullptr : time_wheels_.back();
}

uint32_t TimeWheelScheduler::create_timer(uint64_t when_ms, uint32_t interval_ms, TimerTask task) {
    if (time_wheels_.empty()) {
        return 0;
    }

    static uint32_t global_inc_id{};
    std::lock_guard<std::mutex> lock(mutex_);
    ++global_inc_id;
    greatest_time_wheel()->add_timer(std::make_shared<Timer>(global_inc_id, when_ms, interval_ms, std::move(task)));
    return global_inc_id;
}

uint32_t TimeWheelScheduler::create_timer_at(uint64_t when_ms, TimerTask task) {
    return create_timer(when_ms, 0, std::move(task));
}

uint32_t TimeWheelScheduler::create_timer_after(uint32_t delay_ms, TimerTask task) {
    const uint64_t when_ms = get_now_timestamp() + delay_ms;
    return create_timer(when_ms, 0, std::move(task));
}

uint32_t TimeWheelScheduler::create_timer_every(uint32_t interval_ms, TimerTask task) {
    const uint64_t when_ms = get_now_timestamp() + interval_ms;
    return create_timer(when_ms, interval_ms, std::move(task));
}

void TimeWheelScheduler::cancel_timer(uint32_t timer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    cancel_timer_ids_.insert(timer_id);
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

        TimeWheelPtr time_wheel = least_time_wheel();
        std::list<TimerPtr> slot = time_wheel->take_current_slot();
        for (TimerPtr& timer: slot) {
            if (auto it = cancel_timer_ids_.find(timer->id()); it != cancel_timer_ids_.end()) {
                cancel_timer_ids_.erase(it);
                continue;
            }

            timer->run();
            if (timer->repeated()) {
                timer->update_when_time();
                greatest_time_wheel()->add_timer(timer);
            }
        }
        time_wheel->increase();
    }
}
