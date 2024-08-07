#version 450


layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_uv_coord;

layout(location = 0) out vec4 v_clip_space_pos;


layout(push_constant) uniform U_PC_Mirror {
    mat4 m_proj_view;
    mat4 m_model_mat;
} u_pc_mirror;


void main() {
    const vec4 world_pos = u_pc_mirror.m_model_mat * vec4(i_position, 1);
    const vec4 clip_space_pos = u_pc_mirror.m_proj_view * world_pos;

    gl_Position = clip_space_pos;
    v_clip_space_pos = clip_space_pos;
}
