#version 450


layout(location = 0) out vec2 v_uv_coord;


layout(set = 0, binding = 1) uniform U_PerFrame {
    mat4 m_rotation;
} u_per_frame;


const vec2 POSITIONS[6] = vec2[](
    vec2(-1, -1),
    vec2(-1,  1),
    vec2( 1,  1),
    vec2(-1, -1),
    vec2( 1,  1),
    vec2( 1, -1)
);

const vec2 UV_COORDS[6] = vec2[](
    vec2(0, 0),
    vec2(0, 1),
    vec2(1, 1),
    vec2(0, 0),
    vec2(1, 1),
    vec2(1, 0)
);


void main() {
    gl_Position = u_per_frame.m_rotation * vec4(POSITIONS[gl_VertexIndex], 0, 1);
    v_uv_coord = UV_COORDS[gl_VertexIndex];
}
