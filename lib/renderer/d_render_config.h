#pragma once


namespace dal {

    struct ShaderConfig {
        bool m_volumetric_atmos = true;
        bool m_atmos_dithering = true;
    };


    struct RendererConfig {
        ShaderConfig m_shader;
    };

}
