#include <iostream>
#include <string>
#include <thread>

#include "time_wheel_scheduler.hpp"

std::string timetoStr() {
    char tmp[64] = {0};
    time_t t = time(nullptr);
    tm *tm = localtime(&t);
    sprintf(tmp, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);
    return tmp;
}

int main() {
    // Four level time wheels: Hour, Minute, Secon, Millisecond.
    int timer_step_ms = 50;
    TimeWheelScheduler tws(timer_step_ms);
    // Hour time wheel. 24 scales, 1 scale = 60 * 60 * 1000 ms.
    tws.append_time_wheel(24, 60 * 60 * 1000, "HourTimeWheel");
    // Minute time wheel. 60 scales, 1 scale = 60 * 1000 ms.
    tws.append_time_wheel(60, 60 * 1000, "MinuteTimeWheel");
    // Second time wheel. 60 scales, 1 scale = 1000 ms.
    tws.append_time_wheel(60, 1000, "SecondTimeWheel");
    // Millisecond time wheel. 1000/timer_step_ms scales, 1 scale = timer_step ms.
    tws.append_time_wheel(1000 / timer_step_ms, timer_step_ms, "MillisecondTimeWheel");

    tws.start();
    std::cout << timetoStr() << std::endl;

    tws.create_timer_at(get_now_timestamp() + 10000, []() {
        std::cout << "At now+10s" << std::endl;
    });

    tws.create_timer_after(500, []() {
        std::cout << "After 0.5s" << std::endl;
    });

    auto timer_id = tws.create_timer_every(5000, []() {
        std::cout << "Every 5s: " << timetoStr() << std::endl;
    });

    tws.create_timer_every(30 * 1000, []() {
        std::cout << "Every 30s: " << timetoStr() << std::endl;
    });

    tws.create_timer_every(60 * 1000, []() {
        std::cout << "Every 1m: " << timetoStr() << std::endl;
    });

    tws.create_timer_every(90 * 1000, []() {
        std::cout << "Every 1.5m: " << timetoStr() << std::endl;
    });

    tws.create_timer_every(120 * 1000, []() {
        std::cout << "Every 2m: " << timetoStr() << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(30));
    tws.cancel_timer(timer_id);

    std::this_thread::sleep_for(std::chrono::minutes(20));
    tws.stop();

    return 0;
}
