#version 450


layout(location = 0) in vec2 v_uv_coord;

layout(location = 0) out vec4 f_out_color;


layout(set = 0, binding = 1) uniform sampler2D u_albedo_map_old;

layout(set = 1, binding = 0) uniform U_PerMaterial {
    float m_roughness;
    float m_metallic;
} u_per_material;

layout(set = 1, binding = 1) uniform sampler2D u_albedo_map;


void main() {
    f_out_color = texture(u_albedo_map, v_uv_coord) * u_per_material.m_roughness;
}
