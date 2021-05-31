#version 450

#include "d_lighting.glsl"


layout(location = 0) in vec2 v_device_coord;

layout (location = 0) out vec4 out_color;


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_albedo;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_material;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_normal;

layout(set = 0, binding = 4) uniform U_GlobalLight {
    vec4 m_dlight_direc[2];
    vec4 m_dlight_color[2];
    uint m_dlight_count;
} u_global_light;

layout(set = 0, binding = 5) uniform U_PerFrame_Composition {
    mat4 m_view_inv;
    mat4 m_proj_inv;
} u_per_frame_composition;


vec3 calc_world_pos(const float z) {
    const vec4 clipSpacePosition = vec4(v_device_coord, z, 1);

    vec4 viewSpacePosition = u_per_frame_composition.m_proj_inv * clipSpacePosition;
    viewSpacePosition /= viewSpacePosition.w;

    const vec4 worldSpacePosition = u_per_frame_composition.m_view_inv * viewSpacePosition;

    return worldSpacePosition.xyz;
}


void main() {
    const float depth = subpassLoad(input_depth).x;
    const vec3 normal = subpassLoad(input_normal).xyz;
    const vec3 albedo = subpassLoad(input_albedo).xyz;
    const vec2 material = subpassLoad(input_material).xy;
    const vec3 world_pos = calc_world_pos(depth);

    vec3 light_color = vec3(0.25);

    for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
        light_color += u_global_light.m_dlight_color[i].xyz * max(dot(u_global_light.m_dlight_direc[i].xyz, normal), 0);
    }

    out_color = vec4(albedo * light_color, 1);
}
