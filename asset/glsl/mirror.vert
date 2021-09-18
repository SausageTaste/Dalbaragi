#version 450


layout(location = 0) out vec4 v_clip_space_pos;


layout(push_constant) uniform U_PC_Mirror {
    mat4 m_proj_view;
    mat4 m_model_mat;

    vec4 m_vertices[3];
} u_pc_mirror;


void main() {
    const vec4 world_pos = vec4(u_pc_mirror.m_vertices[gl_VertexIndex].xyz, 1);
    const vec4 clip_space_pos = u_pc_mirror.m_proj_view * world_pos;

    gl_Position = clip_space_pos;
    v_clip_space_pos = clip_space_pos;
}
