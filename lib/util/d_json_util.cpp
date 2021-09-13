#include "d_json_util.h"


namespace dal {

    std::optional<nlohmann::json> try_parse_json(const std::string& buffer) noexcept {
        try {
            return nlohmann::json::parse(buffer);
        }
        catch (...) {
            return std::nullopt;
        }
    }

}
