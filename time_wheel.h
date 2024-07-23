#ifndef TIME_WHEEL_H_
#define TIME_WHEEL_H_

#include <chrono>
#include <string>
#include <memory>
#include <vector>
#include <list>

#include "timer.h"

class TimeWheel {
public:
    TimeWheel(uint32_t scales, uint32_t scale_unit_ms, std::string name = "");

    [[nodiscard]] uint32_t scale_unit_ms() const {
        return scale_unit_ms_;
    }

    [[nodiscard]] uint32_t scales() const {
        return scales_;
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

    [[nodiscard]] int64_t get_current_time() const;

    void add_timer(const TimerPtr& timer);

    void increase();

    std::list<TimerPtr> get_and_clear_current_slot();

private:
    const std::string name_;
    uint32_t current_index_;

    // A time wheel can be devided into multiple scales. A scals has N ms.
    const uint32_t scales_;
    const uint32_t scale_unit_ms_;

    // Every slot corresponds to a scale. Every slot contains the timers.
    std::vector<std::list<TimerPtr> > slots_;

    TimeWheel *less_level_tw_; // Less scale unit.
    TimeWheel *greater_level_tw_; // Greater scale unit.
};

using TimeWheelPtr = std::shared_ptr<TimeWheel>;

inline int64_t get_now_timestamp() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

#endif  // TIME_WHEEL_H_
