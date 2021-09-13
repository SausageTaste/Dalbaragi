#pragma once

#include <optional>
#include <type_traits>

#include <nlohmann/json.hpp>


namespace dal {

    std::optional<nlohmann::json> try_parse_json(const std::string& buffer) noexcept;

    template <typename T>
    T get_json_number_or(const char* const key, const T fallback, const nlohmann::json& json_data) {
        static_assert(std::is_arithmetic<T>::value);

        const auto iter = json_data.find(key);

        if (json_data.end() != iter && iter.value().is_number()) {
            return iter.value();
        }
        else {
            return fallback;
        }
    }

}
