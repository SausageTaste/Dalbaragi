#pragma once

#include <vector>
#include <memory>
#include <cstdlib>

#include "d_log_channel.h"


namespace dal {

    void critical_exit();

    std::shared_ptr<ILogChannel> get_log_channel_cout();


    class LoggerSingleton {

    private:
        std::vector<std::shared_ptr<ILogChannel>> m_outputs;
        LogLevel m_level = LogLevel::info;

    private:
        LoggerSingleton() = default;

    public:
        static auto& inst() noexcept {
            static LoggerSingleton inst;
            return inst;
        }

        void add_channel(std::shared_ptr<ILogChannel> ptr);

        template <typename _ChTyp>
        void emplace_channel() {
            auto ptr = std::shared_ptr<ILogChannel>(new _ChTyp);
            this->add_channel(ptr);
        }

        void flush();

        void put(
            const LogLevel level, const char* const str,
            const int line, const char* const func, const char* const file
        );

        void put_verbose(
            const char* const str,
            const int line, const char* const func, const char* const file
        ) {
            this->put(LogLevel::verbose, str, line, func, file);
        }
        void put_debug(
            const char* const str,
            const int line, const char* const func, const char* const file
        ) {
            this->put(LogLevel::debug, str, line, func, file);
        }
        void put_info(
            const char* const str,
            const int line, const char* const func, const char* const file
        ) {
            this->put(LogLevel::info, str, line, func, file);
        }
        void put_warn(
            const char* const str,
            const int line, const char* const func, const char* const file
        ) {
            this->put(LogLevel::warning, str, line, func, file);
        }
        void put_error(
            const char* const str,
            const int line, const char* const func, const char* const file
        ) {
            this->put(LogLevel::error, str, line, func, file);
        }
        void put_fatal(
            const char* const str,
            const int line, const char* const func, const char* const file
        ) {
            this->put(LogLevel::fatal, str, line, func, file);
        }

    };

}


#define DAL_ENABLE_ASSERT


#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define dalVerbose(str) dal::LoggerSingleton::inst().put_verbose((str), __LINE__, __func__, __FILE__);
#define dalDebug(str)   dal::LoggerSingleton::inst().put_debug((str),   __LINE__, __func__, __FILE__);
#define dalInfo(str)    dal::LoggerSingleton::inst().put_info((str),    __LINE__, __func__, __FILE__);
#define dalWarn(str)    dal::LoggerSingleton::inst().put_warn((str),    __LINE__, __func__, __FILE__);
#define dalError(str)   dal::LoggerSingleton::inst().put_error((str),   __LINE__, __func__, __FILE__);
#define dalFatal(str)   dal::LoggerSingleton::inst().put_fatal((str),   __LINE__, __func__, __FILE__);

#define dalAbort(str) {                                                             \
    dal::LoggerSingleton::inst().put_fatal((str),   __LINE__, __func__, __FILE__);  \
    dal::LoggerSingleton::inst().flush();                                           \
    dal::critical_exit();                                                           \
}


#ifdef DAL_ENABLE_ASSERT
    #define dalAssert(condition) { if (!(condition)) dalAbort("Assertion failed ( " TOSTRING(condition) " ), file " __FILE__ ", line " TOSTRING(__LINE__)); }
    #define dalAssertm(condition, message) { if (!(condition)) dalAbort(message); }
#else
    #define dalAssert(condition) (condition)
    #define dalAssertm(condition, message) (condition)
#endif
