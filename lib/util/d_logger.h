#pragma once

#include <vector>
#include <memory>
#include <cstdlib>

#include "d_log_channel.h"


#define DAL_ENABLE_ASSERT true


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

        void remove_channel(std::shared_ptr<ILogChannel> ptr);

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

    };

}


#define _STRINGIFY(x) #x
#define TOSTRING(x) _STRINGIFY(x)

#define dalLog(log_level, str)   dal::LoggerSingleton::inst().put((log_level), (str), __LINE__, __func__, __FILE__);

#define dalVerbose(str) dalLog(dal::LogLevel::verbose, (str));
#define dalDebug(str)   dalLog(dal::LogLevel::debug,   (str));
#define dalInfo(str)    dalLog(dal::LogLevel::info,    (str));
#define dalWarn(str)    dalLog(dal::LogLevel::warning, (str));
#define dalError(str)   dalLog(dal::LogLevel::error,   (str));
#define dalFatal(str)   dalLog(dal::LogLevel::fatal,   (str));

#define dalAbort(str) {                    \
    dalFatal(str);                         \
    dal::LoggerSingleton::inst().flush();  \
    dal::critical_exit();                  \
}


#if DAL_ENABLE_ASSERT
    #define dalAssert(condition) { if (!(condition)) dalAbort("Assertion failed ( " TOSTRING(condition) " ), file " __FILE__ ", line " TOSTRING(__LINE__)); }
    #define dalAssertm(condition, message) { if (!(condition)) dalAbort(message); }
#else
    #define dalAssert(condition) (condition)
    #define dalAssertm(condition, message) (condition)
#endif
