#version 450

#include "d_lighting.glsl"


layout(location = 0) in vec2 v_uv_coord;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_world_pos;

layout(location = 0) out vec4 out_color;


layout(set = 0, binding = 0) uniform U_PerFrame {
    mat4 m_view;
    mat4 m_proj;
    vec4 m_view_pos;
} u_per_frame;

layout(set = 0, binding = 1) uniform U_GlobalLight {
    mat4 m_dlight_mat[MAX_D_LIGHT_COUNT];
    vec4 m_dlight_direc[MAX_D_LIGHT_COUNT];
    vec4 m_dlight_color[MAX_D_LIGHT_COUNT];

    vec4 m_plight_pos_n_max_dist[MAX_P_LIGHT_COUNT];
    vec4 m_plight_color[MAX_P_LIGHT_COUNT];

    mat4 m_slight_mat[MAX_S_LIGHT_COUNT];
    vec4 m_slight_pos_n_max_dist[MAX_S_LIGHT_COUNT];
    vec4 m_slight_direc_n_fade_start[MAX_S_LIGHT_COUNT];
    vec4 m_slight_color_n_fade_end[MAX_S_LIGHT_COUNT];

    vec4 m_ambient_light;

    vec4 m_dlight_clip_dist;

    uint m_dlight_count;
    uint m_plight_count;
    uint m_slight_count;
} u_global_light;

layout(set = 0, binding = 2) uniform sampler2D u_dlight_shadow_maps[MAX_D_LIGHT_COUNT];
layout(set = 0, binding = 3) uniform sampler2D u_slight_shadow_maps[MAX_S_LIGHT_COUNT];

layout(set = 1, binding = 0) uniform U_PerMaterial {
    float m_roughness;
    float m_metallic;
} u_per_material;

layout(set = 1, binding = 1) uniform sampler2D u_albedo_map;


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

    const vec3 view_direc = normalize(u_per_frame.m_view_pos.xyz - v_world_pos);
    const vec3 F0 = mix(vec3(0.04), albedo, u_per_material.m_metallic);

    vec3 light = albedo * u_global_light.m_ambient_light.xyz;

    for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
        const float shadow = how_much_not_in_shadow_pcf_bilinear(v_world_pos, u_global_light.m_dlight_mat[i], u_dlight_shadow_maps[i]);

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
        ) * shadow;
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

    for (uint i = 0; i < u_global_light.m_slight_count; ++i) {
        const float shadow = how_much_not_in_shadow_pcf_bilinear(v_world_pos, u_global_light.m_slight_mat[i], u_slight_shadow_maps[i]);
        const vec3 frag_to_light_direc = normalize(u_global_light.m_slight_pos_n_max_dist[i].xyz - v_world_pos);

        const float attenuation = calc_slight_attenuation(
            v_world_pos,
            u_global_light.m_slight_pos_n_max_dist[i].xyz,
            -u_global_light.m_slight_direc_n_fade_start[i].xyz,
            u_global_light.m_slight_direc_n_fade_start[i].w,
            u_global_light.m_slight_color_n_fade_end[i].w
        );

        light += calc_pbr_illumination(
            u_per_material.m_roughness,
            u_per_material.m_metallic,
            albedo,
            v_normal,
            F0,
            view_direc,
            frag_to_light_direc,
            1,
            u_global_light.m_slight_color_n_fade_end[i].xyz
        ) * (attenuation * shadow);
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
