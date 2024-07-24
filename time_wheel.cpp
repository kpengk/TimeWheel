#include "time_wheel.hpp"

#include <utility>


TimeWheel::TimeWheel(uint32_t scales, uint32_t scale_unit_ms, std::string name)
    : scales_{scales}
      , scale_unit_ms_{scale_unit_ms}
      , name_{std::move(name)}
      , current_index_{0}
      , slots_(scales)
      , greater_level_tw_{}
      , less_level_tw_{} {
}

uint64_t TimeWheel::get_current_time() const {
    const uint64_t less_tw_time = less_level_tw_ != nullptr ? less_level_tw_->get_current_time() : 0;
    return current_index_ * scale_unit_ms_ + less_tw_time;
}

void TimeWheel::add_timer(TimerPtr timer) {
    const uint64_t less_tw_time = less_level_tw_ != nullptr ? less_level_tw_->get_current_time() : 0;
    const uint64_t diff = timer->when_ms() - get_now_timestamp() + less_tw_time;

    // If the difference is greater than scale unit, the timer can be added into the current time wheel.
    if (diff >= scale_unit_ms_) {
        const size_t n = (current_index_ + diff / scale_unit_ms_) % scales_;
        slots_[n].push_back(std::move(timer));
        return;
    }

    // If the difference is less than scale uint, the timer should be added into less level time wheel.
    if (less_level_tw_ != nullptr) {
        less_level_tw_->add_timer(std::move(timer));
        return;
    }

    // If the current time wheel is the least level, the timer can be added into the current time wheel.
    slots_[current_index_].push_back(std::move(timer));
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
        std::list<TimerPtr> slot = greater_level_tw_->take_current_slot();
        for (TimerPtr& timer: slot) {
            add_timer(std::move(timer));
        }
    }
}

std::list<TimerPtr> TimeWheel::take_current_slot() {
    std::list<TimerPtr> slot = std::move(slots_[current_index_]);
    return slot;
}
