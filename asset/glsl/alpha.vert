#version 450


layout(location = 0) in vec3 i_position;
layout(location = 1) in vec3 i_normal;
layout(location = 2) in vec2 i_uv_coord;

layout(location = 0) out vec2 v_uv_coord;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_world_pos;


layout(set = 0, binding = 0) uniform U_CameraTransform {
    mat4 m_view;
    mat4 m_proj;
    mat4 m_view_inv;
    mat4 m_proj_inv;

    vec4 m_view_pos;

    float m_near, m_far;
} u_cam_transform;

layout(set = 2, binding = 0) uniform U_PerActor {
    mat4 m_model;
} u_per_actor;


void main() {
    const vec4 world_pos = u_per_actor.m_model * vec4(i_position, 1);
    v_world_pos = world_pos.xyz;
    gl_Position = u_cam_transform.m_proj * u_cam_transform.m_view * world_pos;
    v_uv_coord = i_uv_coord;
    v_normal = normalize((u_per_actor.m_model * vec4(i_normal, 0)).xyz);
}
