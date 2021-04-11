#include "d_logger.h"

#include <iostream>


namespace {

    class LogChannel_COUT : public dal::ILogChannel {

    public:
        virtual void put(
            const dal::LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        ) override {
            if (static_cast<int>(level) >= static_cast<int>(dal::LogLevel::warning))
                std::cerr << "[" << dal::get_log_level_str(level) << "] " << str << std::endl;
            else
                std::cout << "[" << dal::get_log_level_str(level) << "] " << str << std::endl;
        }

    };

}


namespace dal {

    void critical_exit() {
        std::exit(EXIT_FAILURE);
    }

    std::shared_ptr<ILogChannel> get_log_channel_cout() {
        return std::shared_ptr<ILogChannel>{ new LogChannel_COUT };
    }

}


namespace dal {

    void LoggerSingleton::add_channel(std::shared_ptr<ILogChannel> ptr) {
        this->m_outputs.push_back(ptr);
    }

    void LoggerSingleton::put(
        const LogLevel level, const char* const str,
        const int line, const char* const func, const char* const file
    ) {
        if (static_cast<int>(level) < static_cast<int>(this->m_level))
            return;

        for (auto& x : this->m_outputs) {
            x->put(level, str, line, func, file);
        }
    }

    void LoggerSingleton::flush() {
        for (auto& x : this->m_outputs) {
            x->flush();
        }
    }

}
