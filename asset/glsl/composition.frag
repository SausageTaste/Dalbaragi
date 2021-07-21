#version 450

#include "d_lighting.glsl"


layout(location = 0) in vec2 v_device_coord;

layout(location = 0) out vec4 out_color;


layout(input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput input_albedo;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput input_material;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput input_normal;


layout(set = 0, binding = 4) uniform U_GlobalLight {
    mat4 m_dlight_mat[2];
    vec4 m_dlight_direc[2];
    vec4 m_dlight_color[2];

    vec4 m_plight_pos_n_max_dist[3];
    vec4 m_plight_color[3];

    mat4 m_slight_mat[3];
    vec4 m_slight_pos_n_max_dist[3];
    vec4 m_slight_direc_n_fade_start[3];
    vec4 m_slight_color_n_fade_end[3];

    vec4 m_ambient_light;

    uint m_dlight_count;
    uint m_plight_count;
    uint m_slight_count;
} u_global_light;

layout(set = 0, binding = 5) uniform U_PerFrame_Composition {
    mat4 m_view_inv;
    mat4 m_proj_inv;
    vec4 m_view_pos;
} u_per_frame_composition;

layout(set = 0, binding = 6) uniform sampler2D u_dlight_shadow_maps[2];
layout(set = 0, binding = 7) uniform sampler2D u_slight_shadow_maps[3];


vec3 calc_world_pos(const float z) {
    const vec4 clipSpacePosition = vec4(v_device_coord, z, 1);

    vec4 viewSpacePosition = u_per_frame_composition.m_proj_inv * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    const vec4 worldSpacePosition = u_per_frame_composition.m_view_inv * viewSpacePosition;

    return worldSpacePosition.xyz;
}

vec3 fix_color(const vec3 color) {
    const float GAMMA = 2.2;
    const float EXPOSURE = 1;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));
    return mapped;
}


void main() {
    const float depth = subpassLoad(input_depth).x;
    const vec3 normal = subpassLoad(input_normal).xyz * 2.0 - 1.0;
    const vec3 albedo = subpassLoad(input_albedo).xyz;
    const vec2 material = subpassLoad(input_material).xy;

    const vec3 world_pos = calc_world_pos(depth);
    const vec3 view_direc = normalize(u_per_frame_composition.m_view_pos.xyz - world_pos);
    const vec3 F0 = mix(vec3(0.04), albedo, material.y);

    vec3 light = albedo * u_global_light.m_ambient_light.xyz;

    for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
        const float shadow = how_much_not_in_shadow_pcf_bilinear(world_pos, u_global_light.m_dlight_mat[i], u_dlight_shadow_maps[i]);

        light += calc_pbr_illumination(
            material.x,
            material.y,
            albedo,
            normal,
            F0,
            view_direc,
            u_global_light.m_dlight_direc[i].xyz,
            1,
            u_global_light.m_dlight_color[i].xyz
        ) * shadow;
    }

    for (uint i = 0; i < u_global_light.m_plight_count; ++i) {
        const vec3 frag_to_light_direc = normalize(u_global_light.m_plight_pos_n_max_dist[i].xyz - world_pos);

        light += calc_pbr_illumination(
            material.x,
            material.y,
            albedo,
            normal,
            F0,
            view_direc,
            frag_to_light_direc,
            1,
            u_global_light.m_plight_color[i].xyz
        );
    }

    for (uint i = 0; i < u_global_light.m_slight_count; ++i) {
        const float shadow = how_much_not_in_shadow_pcf_bilinear(world_pos, u_global_light.m_slight_mat[i], u_slight_shadow_maps[i]);
        const vec3 frag_to_light_direc = normalize(u_global_light.m_slight_pos_n_max_dist[i].xyz - world_pos);

        const float attenuation = calc_slight_attenuation(
            world_pos,
            u_global_light.m_slight_pos_n_max_dist[i].xyz,
            -u_global_light.m_slight_direc_n_fade_start[i].xyz,
            u_global_light.m_slight_direc_n_fade_start[i].w,
            u_global_light.m_slight_color_n_fade_end[i].w
        );

        light += calc_pbr_illumination(
            material.x,
            material.y,
            albedo,
            normal,
            F0,
            view_direc,
            frag_to_light_direc,
            1,
            u_global_light.m_slight_color_n_fade_end[i].xyz
        ) * (attenuation * shadow);
    }

    out_color = vec4(light, 1);

#ifdef DAL_GAMMA_CORRECT
    out_color.xyz = fix_color(out_color.xyz);
#endif

}
