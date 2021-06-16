#version 450

#include "d_lighting.glsl"


layout(location = 0) in vec2 v_uv_coord;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_world_pos;

layout(location = 0) out vec4 out_color;


layout(set = 1, binding = 0) uniform U_PerMaterial {
    float m_roughness;
    float m_metallic;
} u_per_material;

layout(set = 1, binding = 1) uniform sampler2D u_albedo_map;

layout(set = 3, binding = 0) uniform U_GlobalLight {
    vec4 m_dlight_direc[2];
    vec4 m_dlight_color[2];

    vec4 m_plight_pos_n_max_dist[3];
    vec4 m_plight_color[3];

    vec4 m_ambient_light;

    uint m_dlight_count;
    uint m_plight_count;
} u_global_light;

layout(set = 3, binding = 1) uniform U_PerFrame_Alpha {
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
    const vec4 color_texture = texture(u_albedo_map, v_uv_coord);
    const vec3 albedo = color_texture.xyz;
    const float alpha = color_texture.w;

    const vec3 view_direc = normalize(u_per_frame_alpha.m_view_pos.xyz - v_world_pos);
    const vec3 F0 = mix(vec3(0.04), albedo, u_per_material.m_metallic);

    vec3 light = albedo * u_global_light.m_ambient_light.xyz;

    for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
        light += calc_pbr_illumination(
            u_per_material.m_roughness,
            u_per_material.m_metallic,
            albedo,
            v_normal,
            F0,
            view_direc,
            u_global_light.m_dlight_direc[i].xyz,
            1,
            u_global_light.m_dlight_color[i].xyz
        );
    }

    for (uint i = 0; i < u_global_light.m_plight_count; ++i) {
        const vec3 frag_to_light_direc = normalize(u_global_light.m_plight_pos_n_max_dist[i].xyz - v_world_pos);

        light += calc_pbr_illumination(
            u_per_material.m_roughness,
            u_per_material.m_metallic,
            albedo,
            v_normal,
            F0,
            view_direc,
            frag_to_light_direc,
            1,
            u_global_light.m_plight_color[i].xyz
        );
    }

    out_color = vec4(light, alpha);

#ifdef DAL_ALPHA_CLIP
    if (out_albedo.a < 0.5)
        discard;
#endif

#ifdef DAL_GAMMA_CORRECT
    out_color.xyz = fix_color(out_color.xyz);
#endif

}
