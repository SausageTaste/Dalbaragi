#version 450

#include "d_lighting.glsl"


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


void main() {
    const float depth = subpassLoad(input_depth).x;
    const vec3 normal = subpassLoad(input_normal).xyz;
    const vec3 albedo = subpassLoad(input_albedo).xyz;
    const vec2 material = subpassLoad(input_material).xy;

    vec3 light_color = vec3(0.25);

    for (uint i = 0; i < u_global_light.m_dlight_count; ++i) {
        light_color += u_global_light.m_dlight_color[i].xyz * max(dot(u_global_light.m_dlight_direc[i].xyz, normal), 0);
    }

    out_color = vec4(albedo * light_color, 1);
}
