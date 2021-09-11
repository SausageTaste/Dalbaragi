#include "d_atmos_scattering_copied.glsl"

#define DAL_ANISOTROPY_SQR (G * G)
#define DAL_ATMOS_RAD_SQR (ATMOS_RADIUS * ATMOS_RADIUS)
#define DAL_ATMOS_MAX_DIST 1e12

// Simplified version of `calculate_scattering`
vec3 calculate_dlight_scattering_sky(
    const vec3 view_pos,
    const vec3 dir,
    const vec3 light_dir,
    const vec3 light_intensity,
    const vec3 planet_position,
    const vec3 beta_ray,
    const vec3 beta_mie,
    const int steps_i,
    const int steps_l
) {
    // the start of the ray (the camera position)
    // add an offset to the camera position, so that the atmosphere is in the correct position
    const vec3 start = view_pos - planet_position;

    // calculate the start and end position of the ray, as a distance along the ray
    // we do this with a ray sphere intersect
    float a = dot(dir, dir);
    float b = 2.0 * dot(dir, start);
    float c = dot(start, start) - DAL_ATMOS_RAD_SQR;
    float d = (b * b) - 4.0 * a * c;

    // stop early if there is no intersect
    if (d < 0.0)
        return vec3(0);

    // calculate the ray length
    vec2 ray_length = vec2(
        max((-b - sqrt(d)) / (2.0 * a), 0.0),
        min((-b + sqrt(d)) / (2.0 * a), DAL_ATMOS_MAX_DIST)
    );

    // if the ray did not hit the atmosphere, return a black color
    if (ray_length.x > ray_length.y)
        return vec3(0);

    // make sure the ray is no longer than allowed
    ray_length.y = min(ray_length.y, DAL_ATMOS_MAX_DIST);
    ray_length.x = max(ray_length.x, 0.0);
    // get the step size of the ray
    const float step_size_i = (ray_length.y - ray_length.x) / float(steps_i);

    vec3 total_ray = vec3(0.0); // for rayleigh
    vec3 total_mie = vec3(0.0); // for mie
    vec3 opt_i = vec3(0.0);
    vec3 prev_density = vec3(0.0);
    const vec2 scale_height = vec2(HEIGHT_RAY, HEIGHT_MIE);

    const float mu = dot(dir, light_dir);
    const float mumu = mu * mu;
    const float phase_ray = 3.0 / (16.0 * DAL_PI) * (1.0 + mumu);
    const float phase_mie = 3.0 / (8.0 * DAL_PI) * ((1.0 - DAL_ANISOTROPY_SQR) * (mumu + 1.0)) / (pow(1.0 + DAL_ANISOTROPY_SQR - 2.0 * mu * G, 1.5) * (2.0 + DAL_ANISOTROPY_SQR));

    for (int i = 0; i < steps_i; ++i) {

        // calculate where we are along this
        const float index_factor = float(i) + 0.5;
        const vec3 pos_i = start + dir * (ray_length.x + step_size_i *index_factor);

        // and how high we are above the surface
        const float height_i = length(pos_i) - PLANET_RADIUS;

        // now calculate the density of the particles (both for rayleigh and mie)
        vec3 density = vec3(exp(-height_i / scale_height), 0.0);

        // and the absorption density. this is for ozone, which scales together with the rayleigh,
        // but absorbs the most at a specific height, so use the sech function for a nice curve falloff for this height
        // clamp it to avoid it going out of bounds. This prevents weird black spheres on the night side
        const float denom = (HEIGHT_ABSORPTION - height_i) / ABSORPTION_FALLOFF;
        density.z = (1.0 / (denom * denom + 1.0)) * density.x;

        // multiply it by the step size here
        // we are going to use the density later on as well
        density *= step_size_i;

        // Add these densities to the optical depth, so that we know how many particles are on this ray.
        // max here is needed to prevent opt_i from potentially becoming negative
        opt_i += max(density + (prev_density - density) * 0.5, 0.0);

        // and update the previous density
        prev_density = density;

        // Calculate the step size of the light ray.
        // again with a ray sphere intersect
        // a, b, c and d are already defined
        a = dot(light_dir, light_dir);
        b = 2.0 * dot(light_dir, pos_i);
        c = dot(pos_i, pos_i) - DAL_ATMOS_RAD_SQR;
        d = (b * b) - 4.0 * a * c;

        // no early stopping, this one should always be inside the atmosphere
        // calculate the ray length
        const float step_size_l = (-b + sqrt(d)) / (2.0 * a * float(steps_l));

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
            const float height_l = length(pos_l) - PLANET_RADIUS;

            // calculate the particle density, and add it
            // this is a bit verbose
            // first, set the density for ray and mie
            vec3 density_l = vec3(exp(-height_l / scale_height), 0.0);

            // then, the absorption
            const float denom = (HEIGHT_ABSORPTION - height_l) / ABSORPTION_FALLOFF;
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
        const vec3 attn = exp(-beta_ray * (opt_i.x + opt_l.x) - beta_mie * (opt_i.y + opt_l.y) - ABSORPTION_BETA * (opt_i.z + opt_l.z));

        // accumulate the scattered light (how much will be scattered towards the camera)
        total_ray += density.x * attn;
        total_mie += density.y * attn;

    }

    // calculate and return the final color
    return (
          phase_ray * beta_ray * total_ray          // rayleigh color
        + phase_mie * beta_mie * total_mie          // mie
        + opt_i.x   * AMBIENT_BETA                  // and ambient
    ) * light_intensity;
}
