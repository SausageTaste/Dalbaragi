#version 450


layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_uv_coord;

layout(location = 0) out vec3 v_world_pos;
layout(location = 1) out vec2 v_uv_coord;
layout(location = 2) out vec3 v_normal;


layout(push_constant) uniform U_PC_Simple {
    mat4 m_model_mat;
    mat4 m_proj_view_mat;

    vec4 m_clip_plane;
} u_pc;


void main() {
    const vec4 world_pos = u_pc.m_model_mat * vec4(i_position, 1);

    gl_Position = u_pc.m_proj_view_mat * world_pos;
    v_world_pos = world_pos.xyz;
    v_uv_coord = i_uv_coord;
    v_normal = normalize(mat3(u_pc.m_model_mat) * i_normal);
}
