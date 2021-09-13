#include "d_config.h"


namespace dal {

    MainConfig::MainConfig()
        : m_window_width(1280)
        , m_window_height(720)
        , m_startup_script_path()
    {

    }

    void MainConfig::load_json(const uint8_t* const buf, const size_t buf_size) {

    }

    std::vector<uint8_t> MainConfig::export_json() const {
        std::vector<uint8_t> output;

        return output;
    }

}
