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

    float m_atmos_intensity;
    float m_mie_scattering_coeff;

    uint m_dlight_count;
    uint m_plight_count;
    uint m_slight_count;
} u_global_light;

layout(set = 0, binding = 5) uniform U_CameraTransform {
    mat4 m_view;
    mat4 m_proj;
    mat4 m_view_inv;
    mat4 m_proj_inv;

    vec4 m_view_pos;

    float m_near, m_far;
} u_camera_transform;

layout(set = 0, binding = 6) uniform sampler2D u_dlight_shadow_maps[MAX_D_LIGHT_COUNT];
layout(set = 0, binding = 7) uniform sampler2D u_slight_shadow_maps[MAX_S_LIGHT_COUNT];


vec3 calc_world_pos(const float z) {
    const vec4 clip_space_position = vec4(v_device_coord, z, 1);

    vec4 view_space_position = u_camera_transform.m_proj_inv * clip_space_position;
    view_space_position /= view_space_position.w;

    return (u_camera_transform.m_view_inv * view_space_position).xyz;
}

float calc_view_z_of(const vec3 p) {
    const mat4 m = u_camera_transform.m_view;
    return dot(vec3(m[0].z, m[1].z, m[2].z), p) + m[3].z;
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

    const vec3 near_pos = view_pos + (sample_direc * u_camera_transform.m_near);
    const float near_view_z = -u_camera_transform.m_near;

    const vec3 step_pos = (sample_end_dist - u_camera_transform.m_near) / float(NUM_STEPS + 1) * sample_direc;
    const float step_view_z = (calc_view_z(frag_depth, u_camera_transform.m_near, u_camera_transform.m_far) - near_view_z) / float(NUM_STEPS + 1);

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
        //const float sample_depth = calc_depth_of_z(step_view_z * dither_factor + near_view_z, u_camera_transform.m_near, u_camera_transform.m_far);
        const vec3 sample_pos = step_pos * dither_factor + near_pos;
        const float sample_depth = calc_depth_of_z(calc_view_z_of(sample_pos), u_camera_transform.m_near, u_camera_transform.m_far);

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


vec4 calc_ray_sphere_factors(const vec3 ray_start, const vec3 ray_direction, const vec3 sphere_center, const float sphere_radius) {
    const vec3 rel_ray_start = ray_start - sphere_center;
    const float a = dot(ray_direction, ray_direction);
    const float b = dot(ray_direction, rel_ray_start);
    const float c = dot(rel_ray_start, rel_ray_start) - (sphere_radius * sphere_radius);
    const float d = (b * b) - a * c;
    return vec4(a, b, c, d);
}


vec2 calc_ray_sphere_length_pair(const vec4 factors, const float max_distance) {
    return vec2(
        max((-factors.y - sqrt(factors.w)) / (factors.x), 0.0),
        min((-factors.y + sqrt(factors.w)) / (factors.x), max_distance)
    );
}


vec3 calculate_dlight_scattering(
    const vec3 view_pos,            // the view position (the camera position)
    const vec3 dir,                 // the direction of the ray (the camera vector), must be normalized
    const float max_dist,           // the maximum distance the ray can travel (because something is in the way, like an object)
    const vec3 scene_color,         // the color of the scene
    const vec3 light_dir,           // the direction of the light
    const vec3 light_intensity,     // how bright the light is, affects the brightness of the
    const vec3 planet_position,     // the position of the planet
    const float planet_radius, 	    // the radius of the planet
    const float atmo_radius,        // the radius of the atmosphere
    const vec3 beta_ray,            // the amount rayleigh scattering scatters the colors (for earth: causes the blue atmosphere)
    const vec3 beta_mie,            // the amount mie scattering scatters colors
    const vec3 beta_absorption,     // how much air is absorbed
    const vec3 beta_ambient,        // the amount of scattering that always occurs, cna help make the back side of the atmosphere a bit brighter
    const float g,                  // the direction mie scatters the light in (like a cone). closer to -1 means more towards a single direction
    const float height_ray,         // how high do you have to go before there is no rayleigh scattering?
    const float height_mie,         // the same, but for mie
    const float height_absorption,  // the height at which the most absorption happens
    const float absorption_falloff, // how fast the absorption falls off from the absorption height
    const int steps_i,              // the amount of steps along the 'primary' ray, more looks better but slower
    const int steps_l               // the amount of steps along the light ray, more looks better but slower
) {
    // the start of the ray (the camera position)
    // add an offset to the camera position, so that the atmosphere is in the correct position
    const vec3 start = view_pos - planet_position;

    // calculate the start and end position of the ray, as a distance along the ray
    // we do this with a ray sphere intersect
    vec4 abcd1 = calc_ray_sphere_factors(view_pos, dir, planet_position, atmo_radius);

    // stop early if there is no intersect
    if (abcd1[3] < 0.0) return scene_color;

    // calculate the ray length
    vec2 ray_length = calc_ray_sphere_length_pair(abcd1, max_dist);

    // if the ray did not hit the atmosphere, return a black color
    if (ray_length.x > ray_length.y) return scene_color;

    // make sure the ray is no longer than allowed
    ray_length.y = min(ray_length.y, max_dist);
    ray_length.x = max(ray_length.x, 0.0);
    // get the step size of the ray
    const float step_size_i = (ray_length.y - ray_length.x) / float(steps_i);

    // these are the values we use to gather all the scattered light
    vec3 total_ray = vec3(0.0); // for rayleigh
    vec3 total_mie = vec3(0.0); // for mie

    // initialize the optical depth. This is used to calculate how much air was in the ray
    vec3 opt_i = vec3(0.0);

    // we define the density early, as this helps doing integration
    // usually we would do riemans summing, which is just the squares under the integral area
    // this is a bit innefficient, and we can make it better by also taking the extra triangle at the top of the square into account
    // the starting value is a bit inaccurate, but it should make it better overall
    vec3 prev_density = vec3(0.0);

    // also init the scale height, avoids some vec2's later on
    const vec2 scale_height = vec2(height_ray, height_mie);

    // Calculate the Rayleigh and Mie phases.
    // This is the color that will be scattered for this ray
    // mu, mumu and gg are used quite a lot in the calculation, so to speed it up, precalculate them
    const float mu = dot(dir, light_dir);
    const float mumu = mu * mu;
    const float gg = g * g;
    const float phase_ray = 3.0 / (16.0 * DAL_PI) * (1.0 + mumu);

    // prevent the mie glow from appearing if there's an object in front of the camera
#ifdef DAL_VOLUMETRIC_ATMOS
    const float phase_mie = 3.0 / (8.0 * DAL_PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));
#else
    const float phase_mie = (max_dist > ray_length.y) ? 3.0 / (8.0 * DAL_PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg)) : 0.0;
#endif

#ifdef DAL_ATMOS_DITHERING
    const float dither_value = get_dither_value();
#endif

    // now we need to sample the 'primary' ray. this ray gathers the light that gets scattered onto it
    for (int i = 0; i < steps_i; ++i) {

        // calculate where we are along this ray
#ifdef DAL_ATMOS_DITHERING
        const vec3 sample_offset = dir * (ray_length.x + step_size_i * (float(i) + dither_value + 0.5));
#else
        const vec3 sample_offset = dir * (ray_length.x + step_size_i * (float(i) + 0.5));
#endif
        const vec3 world_pos_i   = view_pos + sample_offset;
        const vec3 pos_i         = start + sample_offset;

#ifdef DAL_VOLUMETRIC_ATMOS
        bool in_shadow_i = false;
        const float depth_i = calc_depth_of_z(calc_view_z_of(world_pos_i), u_camera_transform.m_near, u_camera_transform.m_far);

        uint selected_dlight = u_global_light.m_dlight_count - 1;
        for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
            if (u_global_light.m_dlight_clip_dist[i] > depth_i) {
                selected_dlight = i;
                break;
            }
        }

        const vec4 sample_pos_in_dlight = u_global_light.m_dlight_mat[selected_dlight] * vec4(world_pos_i, 1);
        const vec3 proj_coords = sample_pos_in_dlight.xyz / sample_pos_in_dlight.w;
        if (proj_coords.z > 1.0) {
            in_shadow_i = true;
        }
        else {
            if (proj_coords.z >= texture(u_dlight_shadow_maps[selected_dlight], proj_coords.xy * 0.5 + 0.5).r) {
                in_shadow_i = true;
            }
        }
#endif

        // and how high we are above the surface
        const float height_i = length(pos_i) - planet_radius;

        // now calculate the density of the particles (both for rayleigh and mie)
        vec3 density = vec3(exp(-height_i / scale_height), 0.0);

        // and the absorption density. this is for ozone, which scales together with the rayleigh,
        // but absorbs the most at a specific height, so use the sech function for a nice curve falloff for this height
        // clamp it to avoid it going out of bounds. This prevents weird black spheres on the night side
        const float denom = (height_absorption - height_i) / absorption_falloff;
        density.z = (1.0 / (denom * denom + 1.0)) * density.x;

        // multiply it by the step size here
        // we are going to use the density later on as well
        density *= step_size_i;

        // Add these densities to the optical depth, so that we know how many particles are on this ray.
        // max here is needed to prevent opt_i from potentially becoming negative
        opt_i += max(density + (prev_density - density) * 0.5, 0.0);

        // and update the previous density
        prev_density = density;

#ifdef DAL_VOLUMETRIC_ATMOS
        if (!in_shadow_i) {
#endif
            // Calculate the step size of the light ray.
            // again with a ray sphere intersect
            // a, b, c and d are already defined
            const vec4 abcd2 = calc_ray_sphere_factors(pos_i, light_dir, planet_position, atmo_radius);

            // no early stopping, this one should always be inside the atmosphere
            // calculate the ray length
            const float step_size_l = (-abcd2[1] + sqrt(abcd2[3])) / (2.0 * abcd2[0] * float(steps_l));

            // and the position along this ray
            // this time we are sure the ray is in the atmosphere, so set it to 0
            float ray_pos_l = step_size_l * 0.5;

            // and the optical depth of this ray
            vec3 opt_l = vec3(0.0);

            // again, use the prev density for better integration
            vec3 prev_density_l = vec3(0.0);

            // now sample the light ray
            // this is similar to what we did before
            for (int l = 0; l < steps_l; ++l) {

                // calculate where we are along this ray
                const vec3 pos_l = pos_i + light_dir * ray_pos_l;

                // the heigth of the position
                const float height_l = length(pos_l) - planet_radius;

                // calculate the particle density, and add it
                // this is a bit verbose
                // first, set the density for ray and mie
                vec3 density_l = vec3(exp(-height_l / scale_height), 0.0);

                // then, the absorption
                const float denom = (height_absorption - height_l) / absorption_falloff;
                density_l.z = (1.0 / (denom * denom + 1.0)) * density_l.x;

                // multiply the density by the step size
                density_l *= step_size_l;

                // and add it to the total optical depth
                opt_l += max(density_l + (prev_density_l - density_l) * 0.5, 0.0);

                // and update the previous density
                prev_density_l = density_l;

                // and increment where we are along the light ray.
                ray_pos_l += step_size_l;

            }

            // Now we need to calculate the attenuation
            // this is essentially how much light reaches the current sample point due to scattering
            const vec3 attn = exp(-beta_ray * (opt_i.x + opt_l.x) - beta_mie * (opt_i.y + opt_l.y) - beta_absorption * (opt_i.z + opt_l.z));

            // accumulate the scattered light (how much will be scattered towards the camera)
            total_ray += density.x * attn;
            total_mie += density.y * attn;
#ifdef DAL_VOLUMETRIC_ATMOS
        }
#endif

    }

    // calculate how much light can pass through the atmosphere
    const vec3 opacity = exp(-(beta_mie * opt_i.y + beta_ray * opt_i.x + beta_absorption * opt_i.z));

    // calculate and return the final color
    return (
          phase_ray * beta_ray * total_ray          // rayleigh color
        + phase_mie * beta_mie * total_mie          // mie
        + opt_i.x   * beta_ambient                  // and ambient
    ) * light_intensity + scene_color * opacity;    // now make sure the background is rendered correctly
}


void main() {
    const float depth = subpassLoad(input_depth).x;
    const vec3 normal = subpassLoad(input_normal).xyz * 2.0 - 1.0;
    const vec3 albedo = subpassLoad(input_albedo).xyz;
    const vec2 material = subpassLoad(input_material).xy;

    const vec3 world_pos = calc_world_pos(depth);
    const vec3 view_vec = world_pos - u_camera_transform.m_view_pos.xyz;
    const vec3 view_direc = normalize(view_vec);
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
            -view_direc,
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
            -view_direc,
            frag_to_light_direc,
            light_distance,
            u_global_light.m_plight_color[i].xyz
        );
    }

    // Spot lights
    for (uint i = 0; i < u_global_light.m_slight_count; ++i) {
        const float shadow = how_much_not_in_shadow_pcf_bilinear(world_pos, u_global_light.m_slight_mat[i], u_slight_shadow_maps[i]);

        const vec3 frag_to_light = u_global_light.m_slight_pos_n_max_dist[i].xyz - world_pos;
        const float light_distance = length(frag_to_light);
        const vec3 frag_to_light_direc = frag_to_light / light_distance;

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
            -view_direc,
            frag_to_light_direc,
            light_distance,
            u_global_light.m_slight_color_n_fade_end[i].xyz
        ) * (attenuation * shadow);
    }

    out_color = vec4(light, 1);

    if (dot(vec3(0, 1, 0), u_global_light.m_dlight_direc[0].xyz) >= -0.2) {
        //out_color.xyz += calc_scattering(world_pos, depth, u_camera_transform.m_view_pos.xyz);
        //out_color.xyz += clamp(skylight(world_pos, normal, u_global_light.m_dlight_direc[0].xyz, vec3(0.0)) * vec3(0.0, 0.25, 0.05), 0.0, 1.0);

        if (1.0 > depth) {
            out_color.xyz = calculate_dlight_scattering(
                u_camera_transform.m_view_pos.xyz,
                view_direc,
                distance(u_camera_transform.m_view_pos.xyz, world_pos),
                out_color.xyz,
                u_global_light.m_dlight_direc[0].xyz,
                vec3(u_global_light.m_atmos_intensity),
                vec3(u_camera_transform.m_view_pos.x, -PLANET_RADIUS, u_camera_transform.m_view_pos.z),
                PLANET_RADIUS,
                ATMOS_RADIUS,
                RAY_BETA,
                vec3(u_global_light.m_mie_scattering_coeff),
                ABSORPTION_BETA,
                AMBIENT_BETA,
                G,
                HEIGHT_RAY,
                HEIGHT_MIE,
                HEIGHT_ABSORPTION,
                ABSORPTION_FALLOFF,
                8,
                4
            );
        }
        else {
            out_color.xyz = calculate_dlight_scattering_sky(
                u_camera_transform.m_view_pos.xyz,
                view_direc,
                u_global_light.m_dlight_direc[0].xyz,
                vec3(u_global_light.m_atmos_intensity),
                vec3(u_camera_transform.m_view_pos.x, -PLANET_RADIUS, u_camera_transform.m_view_pos.z),
                RAY_BETA,
                vec3(u_global_light.m_mie_scattering_coeff),
                8,
                4
            );
        }
    }

    out_color.xyz = 1.0 - exp(-out_color.xyz);
}
