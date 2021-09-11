#version 450

#include "d_lighting.glsl"
#include "d_atmos_scattering.glsl"


layout(location = 0) in vec2 v_device_coord;

layout(location = 0) out vec4 out_color;


layout(input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout(input_attachment_index = 1, binding = 1) uniform subpassInput input_albedo;
layout(input_attachment_index = 2, binding = 2) uniform subpassInput input_material;
layout(input_attachment_index = 3, binding = 3) uniform subpassInput input_normal;


layout(set = 0, binding = 4) uniform U_GlobalLight {
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

layout(set = 0, binding = 5) uniform U_PerFrame_Composition {
    mat4 m_view_inv;
    mat4 m_proj_inv;
    vec4 m_view_pos;

    float m_near;
    float m_far;
} u_per_frame_composition;

layout(set = 0, binding = 6) uniform sampler2D u_dlight_shadow_maps[MAX_D_LIGHT_COUNT];
layout(set = 0, binding = 7) uniform sampler2D u_slight_shadow_maps[MAX_S_LIGHT_COUNT];


vec3 calc_world_pos(const float z) {
    const vec4 clip_space_position = vec4(v_device_coord, z, 1);

    vec4 view_space_position = u_per_frame_composition.m_proj_inv * clip_space_position;
    view_space_position /= view_space_position.w;

    return (u_per_frame_composition.m_view_inv * view_space_position).xyz;
}

vec3 fix_color(const vec3 color) {
    const float GAMMA = 2.2;
    const float EXPOSURE = 1;

    vec3 mapped = vec3(1.0) - exp(-color * EXPOSURE);
    //vec3 mapped = color / (color + 1.0);
    mapped = pow(mapped, vec3(1.0 / GAMMA));
    return mapped;
}

vec3 calc_scattering(const vec3 frag_pos, const float frag_depth, const vec3 view_pos) {
    const int NUM_STEPS = 5;
    const float INTENSITY_DLIGHT = 0.6;
    const float INTENSITY_SLIGHT = 0.9;
    const float MAX_SAMPLE_DIST = 50.0;

    const vec3 view_to_frag = frag_pos - view_pos;
    const float view_to_frag_dist = length(view_to_frag);
    const vec3 sample_direc = view_to_frag / view_to_frag_dist;
    const float sample_end_dist = min(view_to_frag_dist, MAX_SAMPLE_DIST);

    const vec3 near_pos = view_pos + (sample_direc * u_per_frame_composition.m_near);
    const float near_view_z = -u_per_frame_composition.m_near;

    const vec3 step_pos = (sample_end_dist - u_per_frame_composition.m_near) / float(NUM_STEPS + 1) * sample_direc;
    const float step_view_z = (calc_view_z(frag_depth, u_per_frame_composition.m_near, u_per_frame_composition.m_far) - near_view_z) / float(NUM_STEPS + 1);

#ifdef DAL_VOLUMETRIC_SPOT_LIGHT
    float slight_mie_factors[MAX_S_LIGHT_COUNT];
    for (uint i = 0; i < u_global_light.m_slight_count; ++i)
        slight_mie_factors[i] = phase_mie(dot(sample_direc, u_global_light.m_slight_direc_n_fade_start[i].xyz), 0.3);
#endif

    const float dlight_factor = INTENSITY_DLIGHT * phase_mie(dot(sample_direc, u_global_light.m_dlight_direc[0].xyz), 0.7);
    const float slight_factor = INTENSITY_SLIGHT;
    const float dither_value = get_dither_value();

    float accum_factor = 0;
    uint last_selected_dlight = 0;

    for (uint i = 0; i < NUM_STEPS; ++i) {
        const float dither_factor = float(i) + dither_value;
        const float sample_depth = calc_depth_of_z(step_view_z * dither_factor + near_view_z, u_per_frame_composition.m_near, u_per_frame_composition.m_far);
        const vec3 sample_pos = step_pos * dither_factor + near_pos;

        {
            uint selected_dlight = u_global_light.m_dlight_count - 1;
            for (uint i = last_selected_dlight; i < u_global_light.m_dlight_count; ++i) {
                if (u_global_light.m_dlight_clip_dist[i] > sample_depth) {
                    selected_dlight = i;
                    break;
                }
            }
            last_selected_dlight = selected_dlight;

            const vec4 sample_pos_in_dlight = u_global_light.m_dlight_mat[selected_dlight] * vec4(sample_pos, 1);
            const vec3 proj_coords = sample_pos_in_dlight.xyz / sample_pos_in_dlight.w;
            if (proj_coords.z > 1.0) {
                continue;
            }

            const vec2 sample_coord = proj_coords.xy * 0.5 + 0.5;
            const float closest_depth = texture(u_dlight_shadow_maps[selected_dlight], sample_coord).r;
            const float current_depth = proj_coords.z;

            if (current_depth < closest_depth) {
                accum_factor += dlight_factor;
            }
        }

#ifdef DAL_VOLUMETRIC_SPOT_LIGHT
        for (uint i = 0; i < u_global_light.m_slight_count; ++i) {
            const vec4 sample_pos_in_light = u_global_light.m_slight_mat[i] * vec4(sample_pos, 1);
            const vec3 proj_coords = sample_pos_in_light.xyz / sample_pos_in_light.w;
            if (proj_coords.z > 1.0)
                continue;

            const vec2 sample_coord = proj_coords.xy * 0.5 + 0.5;
            const float closest_depth = texture(u_slight_shadow_maps[i], sample_coord).r;
            const float current_depth = proj_coords.z;

            if (current_depth < closest_depth) {
                const float attenuation = calc_slight_attenuation(
                    sample_pos,
                    u_global_light.m_slight_pos_n_max_dist[i].xyz,
                    -u_global_light.m_slight_direc_n_fade_start[i].xyz,
                    u_global_light.m_slight_direc_n_fade_start[i].w,
                    u_global_light.m_slight_color_n_fade_end[i].w
                );

                accum_factor += slight_factor * attenuation * slight_mie_factors[i];
            }
        }
#endif

    }

    return u_global_light.m_dlight_color[0].xyz * (accum_factor / float(NUM_STEPS));
}


void main() {
    const float depth = subpassLoad(input_depth).x;
    const vec3 normal = subpassLoad(input_normal).xyz * 2.0 - 1.0;
    const vec3 albedo = subpassLoad(input_albedo).xyz;
    const vec2 material = subpassLoad(input_material).xy;

    const vec3 world_pos = calc_world_pos(depth);
    const vec3 view_vec = u_per_frame_composition.m_view_pos.xyz - world_pos;
    const float view_distance = length(view_vec);
    const vec3 view_direc = view_vec / view_distance;
    const vec3 F0 = mix(vec3(0.04), albedo, material.y);

    vec3 light = albedo * u_global_light.m_ambient_light.xyz;

    // Directional lights
    {
        uint selected_dlight = u_global_light.m_dlight_count - 1;
        for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
            if (u_global_light.m_dlight_clip_dist[i] > depth) {
                selected_dlight = i;
                break;
            }
        }

        const float shadow = how_much_not_in_shadow_pcf_bilinear(world_pos, u_global_light.m_dlight_mat[selected_dlight], u_dlight_shadow_maps[selected_dlight]);

        light += calc_pbr_illumination(
            material.x,
            material.y,
            albedo,
            normal,
            F0,
            view_direc,
            u_global_light.m_dlight_direc[selected_dlight].xyz,
            1,
            u_global_light.m_dlight_color[selected_dlight].xyz
        ) * shadow;
    }

    // Point lights
    for (uint i = 0; i < u_global_light.m_plight_count; ++i) {
        const vec3 frag_to_light = u_global_light.m_plight_pos_n_max_dist[i].xyz - world_pos;
        const float light_distance = length(frag_to_light);
        const vec3 frag_to_light_direc = frag_to_light / light_distance;

        light += calc_pbr_illumination(
            material.x,
            material.y,
            albedo,
            normal,
            F0,
            view_direc,
            frag_to_light_direc,
            light_distance,
            u_global_light.m_plight_color[i].xyz
        );
    }

    // Spot lights
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

    if (dot(vec3(0, 1, 0), u_global_light.m_dlight_direc[0].xyz) >= -0.2) {
        //out_color.xyz += calc_scattering(world_pos, depth, u_per_frame_composition.m_view_pos.xyz);
        out_color.xyz += clamp(skylight(world_pos, normal, u_global_light.m_dlight_direc[0].xyz, vec3(0.0)) * vec3(0.0, 0.25, 0.05), 0.0, 1.0);

        out_color.xyz = calculate_scattering(
            u_per_frame_composition.m_view_pos.xyz,
            view_direc,
            (1.0 != depth) ? distance(u_per_frame_composition.m_view_pos.xyz, world_pos) : 1e12,
            out_color.xyz,
            -u_global_light.m_dlight_direc[0].xyz,
            vec3(40.0),
            vec3(u_per_frame_composition.m_view_pos.x, PLANET_RADIUS, u_per_frame_composition.m_view_pos.z),
            PLANET_RADIUS,
            ATMOS_RADIUS,
            RAY_BETA,
            MIE_BETA,
            ABSORPTION_BETA,
            AMBIENT_BETA,
            G,
            HEIGHT_RAY,
            HEIGHT_MIE,
            HEIGHT_ABSORPTION,
            ABSORPTION_FALLOFF,
            PRIMARY_STEPS,
            LIGHT_STEPS
        );
    }

    out_color.xyz = 1.0 - exp(-out_color.xyz);

#ifdef DAL_GAMMA_CORRECT
    out_color.xyz = fix_color(out_color.xyz);
#endif

}
