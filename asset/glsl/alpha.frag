#version 450

#include "d_lighting.glsl"


layout(location = 0) in vec2 v_uv_coord;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 out_color;


layout(set = 1, binding = 0) uniform U_PerMaterial {
    float m_roughness;
    float m_metallic;
} u_per_material;

layout(set = 1, binding = 1) uniform sampler2D u_albedo_map;

layout(set=3, binding=0) uniform U_GlobalLight {
    vec4 m_dlight_direc[2];
    vec4 m_dlight_color[2];

    vec4 m_plight_pos_n_max_dist[3];
    vec4 m_plight_color[3];

    vec4 m_ambient_light;

    uint m_dlight_count;
    uint m_plight_count;
} u_global_light;

layout(set=3, binding=1) uniform U_PerFrame_Alpha {
    vec4 m_view_pos;
} u_per_frame_alpha;


vec3 fix_color(const vec3 color) {
    const float GAMMA = 2.2;
    const float EXPOSURE = 1;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));
    return mapped;
}


void main() {
    out_color = texture(u_albedo_map, v_uv_coord);

    float light_color = 0.25;
    light_color += max(dot(TO_LIGHT_DIRECTION, v_normal), 0) * 0.75;

    out_color.xyz *= light_color;

#ifdef DAL_ALPHA_CLIP
    if (out_albedo.a < 0.5)
        discard;
#endif

#ifdef DAL_GAMMA_CORRECT
    out_color.xyz = fix_color(out_color.xyz);
#endif

}
