#version 450


layout(location = 0) in vec3 v_world_pos;
layout(location = 1) in vec2 v_uv_coord;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec3 v_light;

layout(location = 0) out vec4 out_color;


layout(set = 0, binding = 0) uniform U_PerMaterial {
    float m_roughness;
    float m_metallic;
} u_per_material;

layout(set = 0, binding = 1) uniform sampler2D u_albedo_map;


layout(push_constant) uniform U_PC_Simple {
    mat4 m_model_mat;
    vec4 m_clip_plane;
} u_pc;


void main() {
    if (dot(u_pc.m_clip_plane, vec4(v_world_pos, 1)) < 0)
        discard;

    const vec3 albedo = texture(u_albedo_map, v_uv_coord).xyz;

    out_color = vec4(albedo * v_light, 1);
}
