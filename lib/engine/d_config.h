#pragma once

#include <string>
#include <vector>

#include "d_json_util.h"


namespace dal {

    class IConfigGroup {

    public:
        virtual std::string key_name() const = 0;

        virtual void import_json(const nlohmann::json& json_data) = 0;

        virtual nlohmann::json export_json() const = 0;

    };


    class ConfigGroup_Misc : public  IConfigGroup {

    public:
        uint32_t m_window_width = 1280;
        uint32_t m_window_height = 720;
        bool m_full_screen = false;

        std::string m_startup_script_path = "_asset/script/startup.lua";

    public:
        bool m_volumetric_atmos = true;
        bool m_atmos_dithering = true;

        virtual std::string key_name() const {
            return "misc";
        }

        void import_json(const nlohmann::json& json_data) override;

        nlohmann::json export_json() const override;

    };


    class MainConfig {

    public:
        ConfigGroup_Misc m_misc;

    public:
        void load_json(const uint8_t* const buf, const size_t buf_size);

        std::string export_str() const;

    };

}
