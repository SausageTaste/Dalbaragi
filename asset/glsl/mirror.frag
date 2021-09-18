#version 450


layout(location = 0) in vec2 v_uv_coord;
layout(location = 1) in vec3 v_normal;

layout(location = 0) out vec4 out_color;


void main() {
    out_color = vec4(v_normal * 0.5 + 0.5, 1);
}
