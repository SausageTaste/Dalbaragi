#version 450


const vec2 positions[6] = vec2[](
    vec2(-1, -1),
    vec2(-1,  1),
    vec2( 1,  1),
    vec2(-1, -1),
    vec2( 1,  1),
    vec2( 1, -1)
);


layout(location = 0) out vec2 v_device_coord;


void main() {
    v_device_coord = positions[gl_VertexIndex];
    gl_Position = vec4(v_device_coord, 0, 1);
}
