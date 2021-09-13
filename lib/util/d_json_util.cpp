#include "d_json_util.h"


namespace dal {

    std::optional<nlohmann::json> try_parse_json(const std::string& buffer) noexcept {
        if (buffer.empty())
            return std::nullopt;

        try {
            return nlohmann::json::parse(buffer);
        }
        catch (...) {
            return std::nullopt;
        }
    }

    std::optional<nlohmann::json> try_parse_json(const uint8_t* const buf, const size_t buf_size) noexcept {
        if (nullptr == buf)
            return std::nullopt;
        if (0 == buf_size)
            return std::nullopt;

        try {
            return nlohmann::json::parse(buf, buf + buf_size);
        }
        catch (...) {
            return std::nullopt;
        }
    }

}
