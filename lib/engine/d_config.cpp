#include "d_config.h"

#include "d_json_util.h"


namespace {

    const char* const DEFAULT_STARTUP_SCRIPT_PATH = "_asset/script/startup.lua";

    const char* const KEY_WINDOW_WIDTH = "window_width";
    const char* const KEY_WINDOW_HEIGHT = "window_height";
    const char* const KEY_FULL_SCREEN = "full_screen";
    const char* const KEY_STARTUP_SCRIPT_PATH = "startup_script_path";

}


namespace dal {

    MainConfig::MainConfig()
        : m_window_width(1280)
        , m_window_height(720)
        , m_full_screen(false)
        , m_startup_script_path(DEFAULT_STARTUP_SCRIPT_PATH)
    {

    }

    void MainConfig::load_json(const uint8_t* const buf, const size_t buf_size) {
        const auto json_data = try_parse_json(buf, buf_size);
        if (!json_data)
            return;

        try_set_json_value(this->m_window_width, KEY_WINDOW_WIDTH, *json_data);
        try_set_json_value(this->m_window_height, KEY_WINDOW_HEIGHT, *json_data);
        try_set_json_value(this->m_full_screen, KEY_FULL_SCREEN, *json_data);
        try_set_json_value(this->m_startup_script_path, KEY_STARTUP_SCRIPT_PATH, *json_data);
    }

    std::string MainConfig::export_json() const {
        std::vector<uint8_t> output;

        nlohmann::json json_data;
        json_data[KEY_WINDOW_WIDTH] = this->m_window_width;
        json_data[KEY_WINDOW_HEIGHT] = this->m_window_height;
        json_data[KEY_FULL_SCREEN] = this->m_full_screen;
        json_data[KEY_STARTUP_SCRIPT_PATH] = this->m_startup_script_path;

        return json_data.dump(4);
    }

}
