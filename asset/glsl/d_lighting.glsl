
const float PI = 3.14159265359;
const vec3 TO_LIGHT_DIRECTION = normalize(vec3(1, 1, 1));


float calc_slight_attenuation(vec3 frag_pos, vec3 light_pos, vec3 light_direc, float fade_start, float fade_end) {
    const vec3 fragToLight_n = normalize(light_pos - frag_pos);
    const float theta        = dot(-fragToLight_n, light_direc);
    const float epsilon      = fade_start - fade_end;
    const float attenFactor  = 1;

    return clamp((theta - fade_end) / epsilon * attenFactor, 0.0, 1.0);
}


float _distribution_GGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.00001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float _geometry_SchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float _geometry_Smith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = _geometry_SchlickGGX(NdotV, roughness);
    float ggx1 = _geometry_SchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 _fresnel_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}


vec3 calc_pbr_illumination(float roughness, float metallic, vec3 albedo, vec3 normal, vec3 F0, vec3 view_direc, vec3 frag_to_light_direc, float light_distance, vec3 light_color) {
    // calculate per-light radiance
    vec3 L = frag_to_light_direc;
    vec3 H = normalize(view_direc + L);
    float dist = light_distance;
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = light_color * attenuation;

    // Cook-Torrance BRDF
    float NDF = _distribution_GGX(normal, H, roughness);
    float G   = _geometry_Smith(normal, view_direc, L, roughness);
    vec3 F    = _fresnel_Schlick(clamp(dot(H, view_direc), 0.0, 1.0), F0);

    vec3 nominator    = NDF * G * F;
    float denominator = 4 * max(dot(normal, view_direc), 0.0) * max(dot(normal, L), 0.0);
    vec3 specular = nominator / max(denominator, 0.00001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

    // kS is equal to Fresnel
    vec3 kS = F;
    // for energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    vec3 kD = vec3(1.0) - kS;
    // multiply kD by the inverse metalness such that only non-metals
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    kD *= 1.0 - metallic;

    // scale light by NdotL
    float NdotL = max(dot(normal, L), 0.0);

    // add to outgoing radiance Lo
    return (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
}
