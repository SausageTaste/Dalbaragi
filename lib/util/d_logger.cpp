#include "d_logger.h"


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

}
