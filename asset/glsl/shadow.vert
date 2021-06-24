#version 450


layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_uv_coord;


layout(push_constant) uniform PushConstant {
    mat4 m_model_mat;
    mat4 m_light_mat;
} u_pc;


void main() {
    gl_Position = u_pc.m_light_mat * u_pc.m_model_mat * vec4(i_position, 1.0);
}
