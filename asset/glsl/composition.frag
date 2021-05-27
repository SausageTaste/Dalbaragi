#version 450

#include "d_lighting.glsl"


layout (input_attachment_index = 0, binding = 0) uniform subpassInput input_depth;
layout (input_attachment_index = 1, binding = 1) uniform subpassInput input_albedo;
layout (input_attachment_index = 2, binding = 2) uniform subpassInput input_material;
layout (input_attachment_index = 3, binding = 3) uniform subpassInput input_normal;


layout (location = 0) out vec4 out_color;


void main() {
    const float depth = subpassLoad(input_depth).x;
    const vec3 normal = subpassLoad(input_normal).xyz;
    const vec3 albedo = subpassLoad(input_albedo).xyz;
    const vec2 material = subpassLoad(input_material).xy;

    float light_color = 0.25;
    light_color += max(dot(TO_LIGHT_DIRECTION, normal), 0) * 0.75;

    out_color = vec4(albedo * light_color, 1);
}
