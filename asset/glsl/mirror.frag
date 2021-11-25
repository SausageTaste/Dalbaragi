#version 450


layout(location = 0) in vec4 v_clip_space_pos;

layout(location = 0) out vec4 out_color;


layout(set = 0, binding = 0) uniform sampler2D u_reflection_map;


void main() {
    vec2 sceen_uv = (v_clip_space_pos.xy / v_clip_space_pos.w) / 2.0 + 0.5;
    out_color = vec4(texture(u_reflection_map, sceen_uv).xyz, 1);
    out_color = vec4(1, 1, 1, 0.2);
}
