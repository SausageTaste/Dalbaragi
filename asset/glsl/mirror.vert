#version 450


layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_uv_coord;

layout(location = 0) out vec2 v_uv_coord;
layout(location = 1) out vec3 v_normal;


layout(push_constant) uniform U_PC_Mirror {
    mat4 m_proj_view;
    mat4 m_model_mat;
} u_pc_mirror;


const vec2 positions[6] = vec2[](
    vec2(-1, -1),
    vec2(-1,  1),
    vec2( 1,  1),
    vec2(-1, -1),
    vec2( 1,  1),
    vec2( 1, -1)
);


void main() {
    const vec4 world_pos = u_pc_mirror.m_model_mat * vec4(positions[gl_VertexIndex], 0, 1);

    gl_Position = u_pc_mirror.m_proj_view * world_pos;
    v_uv_coord = i_uv_coord;
    v_normal = normalize(mat3(u_pc_mirror.m_model_mat) * i_normal);
}
