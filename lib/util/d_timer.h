#pragma once

#include <chrono>
#include <string>


namespace dal {

    double get_cur_sec(void);

    void sleep_for(const double v);


    class Timer {

    private:
        std::chrono::steady_clock::time_point m_last_checked = std::chrono::steady_clock::now();

    public:
        void check(void);

        double get_elapsed(void) const;

        double check_get_elapsed(void);

    protected:
        auto& last_checked(void) const {
            return this->m_last_checked;
        }

    };


    class TimerThatCaps : public Timer {

    private:
        uint32_t m_desired_delta_microsec = 0;

    public:
        bool has_elapsed(const double sec) const;

        double check_get_elapsed_cap_fps(void);

        void set_fps_cap(const uint32_t v);

    private:
        void wait_to_cap_fps(void);

    };

}
