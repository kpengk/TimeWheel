#pragma once

#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <list>

#include "timer_task_entry.hpp"

inline uint64_t get_now_timestamp() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

class TimeWheel {
public:
    TimeWheel(uint32_t scales, uint32_t scale_unit_ms, std::string name = "")
    : scales_{scales}, scale_unit_ms_{scale_unit_ms}, name_{std::move(name)}, current_index_{0}, slots_(scales),
    greater_level_tw_{}, less_level_tw_{} {
    }

    [[nodiscard]] uint32_t scales() const {
        return scales_;
    }

    [[nodiscard]] uint32_t scale_unit_ms() const {
        return scale_unit_ms_;
    }

    [[nodiscard]] const std::string& name() const {
        return name_;
    }

    [[nodiscard]] uint32_t current_index() const {
        return current_index_;
    }

    void set_less_level_tw(TimeWheel *less_level_tw) {
        less_level_tw_ = less_level_tw;
    }

    void set_greater_level_tw(TimeWheel *greater_level_tw) {
        greater_level_tw_ = greater_level_tw;
    }

    [[nodiscard]] uint64_t get_current_time() const {
        const uint64_t less_tw_time = less_level_tw_ != nullptr ? less_level_tw_->get_current_time() : 0;
        return current_index_ * scale_unit_ms_ + less_tw_time;
    }

    void add_timer(TimerTaskEntryPtr timer){
        const uint64_t less_tw_time = less_level_tw_ != nullptr ? less_level_tw_->get_current_time() : 0;
        const auto diff = static_cast<int64_t>(timer->when_ms() - get_now_timestamp() + less_tw_time);

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

    void increase(){
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
            std::list<TimerTaskEntryPtr> slot = greater_level_tw_->take_current_slot();
            for (TimerTaskEntryPtr& timer: slot) {
                add_timer(std::move(timer));
            }
        }
    }

    std::list<TimerTaskEntryPtr> take_current_slot(){
        std::list<TimerTaskEntryPtr> slot = std::move(slots_[current_index_]);
        return slot;
    }

private:
    // A time wheel can be devided into multiple scales. A scals has N ms.
    const uint32_t scales_;
    const uint32_t scale_unit_ms_;
    const std::string name_;
    uint32_t current_index_;

    // Every slot corresponds to a scale. Every slot contains the timers.
    std::vector<std::list<TimerTaskEntryPtr>> slots_;

    TimeWheel *greater_level_tw_; // Greater scale unit.
    TimeWheel *less_level_tw_; // Less scale unit.
};

using TimeWheelPtr = std::shared_ptr<TimeWheel>;

