
const float PI = 3.14159265359;
const vec3 TO_LIGHT_DIRECTION = normalize(vec3(1, 1, 1));


float calc_slight_attenuation(const vec3 frag_pos, const vec3 light_pos, const vec3 light_direc, const float fade_start, const float fade_end) {
    const vec3 fragToLight_n = normalize(light_pos - frag_pos);
    const float theta        = dot(-fragToLight_n, light_direc);
    const float epsilon      = fade_start - fade_end;
    const float attenFactor  = 1;

    return clamp((theta - fade_end) / epsilon * attenFactor, 0.0, 1.0);
}


bool is_in_shadow(const vec3 world_pos, const mat4 light_mat, sampler2D depth_map) {
    const vec4 frag_pos_in_dlight = light_mat * vec4(world_pos, 1);
    const vec3 proj_coords = frag_pos_in_dlight.xyz / frag_pos_in_dlight.w;
    if (proj_coords.z > 1.0)
        return false;

    const vec2 sample_coord = proj_coords.xy * 0.5 + 0.5;
    const float closestDepth = texture(depth_map, sample_coord).r;
    const float currentDepth = proj_coords.z;

    return currentDepth > closestDepth;
}

float how_much_not_in_shadow(const vec3 world_pos, const mat4 light_mat, sampler2D depth_map) {
    return is_in_shadow(world_pos, light_mat, depth_map) ? 0.0 : 1.0;
}

float how_much_not_in_shadow_pcf(const vec3 world_pos, const mat4 light_mat, sampler2D depth_map) {
    const vec4 frag_pos_in_dlight = light_mat * vec4(world_pos, 1);
    const vec3 proj_coords = frag_pos_in_dlight.xyz / frag_pos_in_dlight.w;
    if (proj_coords.z > 1.0)
        return 1.0;

    const vec2 sample_coord = proj_coords.xy * 0.5 + 0.5;
    const float currentDepth = proj_coords.z;

    float shadow = 0.0;
    const vec2 texel_size = 1.0 / textureSize(depth_map, 0);
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcf_depth = texture(depth_map, sample_coord + vec2(x, y) * texel_size).r;
            shadow += currentDepth > pcf_depth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return 1.0 - shadow;
}


float _distribution_GGX(const vec3 N, const vec3 H, const float roughness) {
    const float a = roughness*roughness;
    const float a2 = a*a;
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotH2 = NdotH*NdotH;

    const float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.00001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float _geometry_SchlickGGX(const float NdotV, const float roughness) {
    const float r = (roughness + 1.0);
    const float k = (r*r) / 8.0;

    const float nom   = NdotV;
    const float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float _geometry_Smith(const vec3 N, const vec3 V, const vec3 L, const float roughness) {
    const float NdotV = max(dot(N, V), 0.0);
    const float NdotL = max(dot(N, L), 0.0);
    const float ggx2 = _geometry_SchlickGGX(NdotV, roughness);
    const float ggx1 = _geometry_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 _fresnel_Schlick(const float cosTheta, const vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}


vec3 calc_pbr_illumination(
    const float roughness,
    const float metallic,
    const vec3 albedo,
    const vec3 normal,
    const vec3 F0,
    const vec3 view_direc,
    const vec3 frag_to_light_direc,
    const float light_distance,
    const vec3 light_color
) {
    // calculate per-light radiance
    const vec3 L = frag_to_light_direc;
    const vec3 H = normalize(view_direc + L);
    const float dist = light_distance;
    const float attenuation = 1.0 / (dist * dist);
    const vec3 radiance = light_color * attenuation;

    // Cook-Torrance BRDF
    const float NDF = _distribution_GGX(normal, H, roughness);
    const float G   = _geometry_Smith(normal, view_direc, L, roughness);
    const vec3 F    = _fresnel_Schlick(clamp(dot(H, view_direc), 0.0, 1.0), F0);

    const vec3 nominator    = NDF * G * F;
    const float denominator = 4 * max(dot(normal, view_direc), 0.0) * max(dot(normal, L), 0.0);
    const vec3 specular = nominator / max(denominator, 0.00001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    // kS is equal to Fresnel
    const vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // scale light by NdotL
    const float NdotL = max(dot(normal, L), 0.0);

    // add to outgoing radiance Lo
    return (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}
