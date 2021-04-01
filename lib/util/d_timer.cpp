#include "d_timer.h"

#include <thread>


namespace {

    void sleep_hot_until(const std::chrono::time_point<std::chrono::steady_clock>& until) {
        while ( std::chrono::steady_clock::now() < until ) {};
    }

    void sleep_cold_until(const std::chrono::time_point<std::chrono::steady_clock>& until) {
        std::this_thread::sleep_until(until);
    }

    void sleep_hybrid_until(const std::chrono::time_point<std::chrono::steady_clock>& until) {
        const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(until - std::chrono::steady_clock::now()).count() * 1 /4;
        std::this_thread::sleep_for(std::chrono::microseconds{ delta });
        ::sleep_hot_until(until);
    }


    constexpr unsigned MISCROSEC_PER_SEC = 1000000;
    constexpr unsigned NANOSEC_PER_SEC = 1000000000;

}


// Header functions
namespace dal {

    double get_cur_sec(void) {
        return static_cast<double>(std::chrono::steady_clock::now().time_since_epoch().count()) / static_cast<double>(NANOSEC_PER_SEC);
    }

    void sleep_for(const double v) {
        auto until = std::chrono::steady_clock::now();
        until += std::chrono::microseconds{ static_cast<uint64_t>(v * static_cast<double>(MISCROSEC_PER_SEC)) };
        ::sleep_hot_until(until);
    }

}


// Timer
namespace dal {

    void Timer::check(void) {
        this->m_last_checked = std::chrono::steady_clock::now();
    }

    double Timer::get_elapsed(void) const {
        const auto deltaTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - this->m_last_checked).count();
        return static_cast<double>(deltaTime_microsec) / static_cast<double>(MISCROSEC_PER_SEC);
    }

    double Timer::check_get_elapsed(void) {
        const auto now = std::chrono::steady_clock::now();
        const auto deltaTime_microsec = std::chrono::duration_cast<std::chrono::microseconds>(now - this->m_last_checked).count();
        this->m_last_checked = now;

        return static_cast<double>(deltaTime_microsec) / static_cast<double>(MISCROSEC_PER_SEC);
    }

}


// TimerThatCaps
namespace dal {

    double TimerThatCaps::check_get_elapsed_cap_fps(void) {
        this->wait_to_cap_fps();
        return this->check_get_elapsed();
    }

    bool TimerThatCaps::has_elapsed(const double sec) const {
        return this->get_elapsed() >= sec;
    }

    void TimerThatCaps::set_fps_cap(const uint32_t v) {
        if (0 != v) {
            this->m_desired_delta_microsec = MISCROSEC_PER_SEC / v;
        }
        else {
            this->m_desired_delta_microsec = 0;
        }
    }

    // Private

    void TimerThatCaps::wait_to_cap_fps(void) {
        const auto wakeTime = this->last_checked() + std::chrono::microseconds{ this->m_desired_delta_microsec };
        ::sleep_hybrid_until(wakeTime);
    }

}
