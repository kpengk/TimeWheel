#include "time_wheel.h"

#include <utility>


TimeWheel::TimeWheel(uint32_t scales, uint32_t scale_unit_ms, std::string name)
    : name_{std::move(name)}
      , current_index_{0}
      , scales_{scales}
      , scale_unit_ms_{scale_unit_ms}
      , slots_(scales)
      , less_level_tw_{}
      , greater_level_tw_{} {
}

int64_t TimeWheel::get_current_time() const {
    int64_t time = current_index_ * scale_unit_ms_;
    if (less_level_tw_ != nullptr) {
        time += less_level_tw_->get_current_time();
    }
    return time;
}

void TimeWheel::add_timer(const TimerPtr& timer) {
    int64_t less_tw_time = 0;
    if (less_level_tw_ != nullptr) {
        less_tw_time = less_level_tw_->get_current_time();
    }
    const uint64_t diff = timer->when_ms() + less_tw_time - get_now_timestamp();

    // If the difference is greater than scale unit, the timer can be added into the current time wheel.
    if (diff >= scale_unit_ms_) {
        size_t n = (current_index_ + diff / scale_unit_ms_) % scales_;
        slots_[n].push_back(timer);
        return;
    }

    // If the difference is less than scale uint, the timer should be added into less level time wheel.
    if (less_level_tw_ != nullptr) {
        less_level_tw_->add_timer(timer);
        return;
    }

    // If the current time wheel is the least level, the timer can be added into the current time wheel.
    slots_[current_index_].push_back(timer);
}

void TimeWheel::increase() {
    // Increase the time wheel.
    ++current_index_;
    if (current_index_ < scales_) {
        return;
    }

    // If the time wheel is full, the greater level time wheel should be increased.
    // The timers in the current slot of the greater level time wheel should be moved into
    // the less level time wheel.
    current_index_ = current_index_ % scales_;
    if (greater_level_tw_ != nullptr) {
        greater_level_tw_->increase();
        const std::list<TimerPtr> slot = std::move(greater_level_tw_->get_and_clear_current_slot());
        for (const TimerPtr& timer: slot) {
            add_timer(timer);
        }
    }
}

std::list<TimerPtr> TimeWheel::get_and_clear_current_slot() {
    std::list<TimerPtr> slot = std::move(slots_[current_index_]);
    return slot;
}
