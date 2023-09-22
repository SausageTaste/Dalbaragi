#include "d_log_channel.h"


namespace dal {

    const char* get_log_level_str(const LogLevel level) {
        switch (level) {
            case LogLevel::verbose:
                return "verbo";
            case LogLevel::info:
                return "infor";
            case LogLevel::debug:
                return "debug";
            case LogLevel::warning:
                return "warng";
            case LogLevel::error:
                return "error";
            case LogLevel::fatal:
                return "fatal";
            default:
                return "?????";
        }
    }

}
