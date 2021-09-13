#include "d_config.h"

#include <nlohmann/json.hpp>


namespace {

    const char* const KEY_WINDOW_WIDTH = "window_width";
    const char* const KEY_WINDOW_HEIGHT = "window_height";
    const char* const KEY_STARTUP_SCRIPT_PATH = "startup_script_path";

}


namespace dal {

    MainConfig::MainConfig()
        : m_window_width(1280)
        , m_window_height(720)
        , m_startup_script_path()
    {

    }

    void MainConfig::load_json(const uint8_t* const buf, const size_t buf_size) {
        if (0 == buf_size)
            return;

        const auto json_data = nlohmann::json::parse(buf, buf + buf_size);

        this->m_window_width = json_data[KEY_WINDOW_WIDTH];
        this->m_window_height = json_data[KEY_WINDOW_HEIGHT];
        this->m_startup_script_path = json_data[KEY_STARTUP_SCRIPT_PATH];
    }

    std::string MainConfig::export_json() const {
        std::vector<uint8_t> output;

        nlohmann::json json_data;
        json_data[KEY_WINDOW_WIDTH] = this->m_window_width;
        json_data[KEY_WINDOW_HEIGHT] = this->m_window_height;
        json_data[KEY_STARTUP_SCRIPT_PATH] = this->m_startup_script_path;

        return json_data.dump(4);
    }

}
