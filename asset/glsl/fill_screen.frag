#version 450


layout(location = 0) in vec2 v_uv_coord;

layout(location = 0) out vec4 f_out_color;

layout(set = 0, binding = 0) uniform sampler2D u_color_map;


void main() {
    f_out_color = texture(u_color_map, v_uv_coord);
}
