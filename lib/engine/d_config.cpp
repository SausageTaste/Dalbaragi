#include "d_config.h"


// ConfigGroup_Misc
namespace {

    const char* const KEY_WINDOW_WIDTH = "window_width";
    const char* const KEY_WINDOW_HEIGHT = "window_height";
    const char* const KEY_FULL_SCREEN = "full_screen";
    const char* const KEY_STARTUP_SCRIPT_PATH = "startup_script_path";

}
namespace dal {

    void ConfigGroup_Misc::import_json(const nlohmann::json& json_data) {
        try_set_json_value(this->m_window_width, KEY_WINDOW_WIDTH, json_data);
        try_set_json_value(this->m_window_height, KEY_WINDOW_HEIGHT, json_data);
        try_set_json_value(this->m_full_screen, KEY_FULL_SCREEN, json_data);
        try_set_json_value(this->m_startup_script_path, KEY_STARTUP_SCRIPT_PATH, json_data);
    }

    nlohmann::json ConfigGroup_Misc::export_json() const {
        nlohmann::json output;

        output[KEY_WINDOW_WIDTH] = this->m_window_width;
        output[KEY_WINDOW_HEIGHT] = this->m_window_height;
        output[KEY_FULL_SCREEN] = this->m_full_screen;
        output[KEY_STARTUP_SCRIPT_PATH] = this->m_startup_script_path;

        return output;
    }

}


namespace {

    auto make_config_group_list(dal::MainConfig& config) {
        return std::vector<dal::IConfigGroup*>{
            &config.m_misc,
        };
    }

    auto make_config_group_list(const dal::MainConfig& config) {
        return std::vector<const dal::IConfigGroup*>{
            &config.m_misc,
        };
    }

}
namespace dal {

    void MainConfig::load_json(const uint8_t* const buf, const size_t buf_size) {
        const auto json_data = try_parse_json(buf, buf_size);
        if (!json_data)
            return;

        for (auto x : ::make_config_group_list(*this)) {
            const auto iter = json_data->find(x->key_name());
            if (json_data->end() != iter) {
                x->import_json(iter.value());
            }
        }
    }

    std::string MainConfig::export_str() const {
        nlohmann::json json_data;

        for (auto x : ::make_config_group_list(*this)) {
            json_data[x->key_name()] = x->export_json();
        }

        return json_data.dump(4);
    }

}
