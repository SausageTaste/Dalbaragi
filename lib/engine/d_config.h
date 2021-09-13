#pragma once

#include <string>


namespace dal {

    class MainConfig {

    public:
        uint32_t m_window_width;
        uint32_t m_window_height;
        bool m_full_screen;

        std::string m_startup_script_path;

    public:
        MainConfig();

        void load_json(const uint8_t* const buf, const size_t buf_size);

        std::string export_json() const;

    };

}
