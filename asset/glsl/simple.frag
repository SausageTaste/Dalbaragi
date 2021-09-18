#version 450


layout(location = 0) in vec3 v_world_pos;
layout(location = 1) in vec2 v_uv_coord;
layout(location = 2) in vec3 v_normal;

layout(location = 0) out vec4 out_color;


layout(push_constant) uniform U_PC_Simple {
    mat4 m_model_mat;
    mat4 m_proj_view_mat;

    vec4 m_clip_plane;
} u_pc;


void main() {
    if (dot(u_pc.m_clip_plane, vec4(v_world_pos, 1)) < 0)
        discard;

    out_color = vec4(v_normal * 0.5 + 0.5, 1);
}
