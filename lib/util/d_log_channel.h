#pragma once


namespace dal {

    enum class LogLevel {
        verbose,
        info,
        debug,
        warning,
        error,
        fatal,
    };

    const char* get_log_level_str(const LogLevel level);


    class ILogChannel {

    public:
        virtual void put(
            const dal::LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        ) = 0;

        virtual void flush() {}

    };

}
