#version 450


layout(location = 0) in vec2 v_uv_coord;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_material;
layout(location = 2) out vec4 out_normal;


layout(set = 1, binding = 0) uniform U_PerMaterial {
    float m_roughness;
    float m_metallic;
} u_per_material;

layout(set = 1, binding = 1) uniform sampler2D u_albedo_map;


void main() {
    out_albedo = texture(u_albedo_map, v_uv_coord);

#ifdef DAL_ALPHA_CLIP
    if (out_albedo.a < 0.5)
        discard;
#endif

    out_material = vec4(u_per_material.m_roughness, u_per_material.m_metallic, 0, 1);
    out_normal = vec4(normalize(v_normal) * 0.5 + 0.5, 0);
}
