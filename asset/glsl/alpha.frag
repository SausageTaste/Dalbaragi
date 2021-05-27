#version 450


layout(location = 0) in vec2 v_uv_coord;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 out_color;


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
    out_color = texture(u_albedo_map, v_uv_coord);

#ifdef DAL_ALPHA_CLIP
    if (out_albedo.a < 0.5)
        discard;
#endif

#ifdef DAL_GAMMA_CORRECT
    out_color.xyz = fix_color(out_color.xyz);
#endif

}