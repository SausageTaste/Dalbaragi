#version 450


layout(location = 0) in ivec4 i_joint_ids;
layout(location = 1) in vec4 i_joint_weights;
layout(location = 2) in vec3 i_position;
layout(location = 3) in vec3 i_normal;
layout(location = 4) in vec2 i_uv_coord;

layout(location = 0) out vec2 v_uv_coord;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_world_pos;


layout(set = 0, binding = 0) uniform U_PerFrame {
    mat4 m_view;
    mat4 m_proj;
    vec4 m_view_pos;
} u_per_frame;

layout(set = 2, binding = 0) uniform U_PerActor {
    mat4 m_model;
} u_per_actor;

layout(set = 3, binding = 0) uniform U_AnimTransform {
    mat4 m_transforms[128];
} u_anim_transform;


mat4 make_joint_transform() {
    mat4 bone_mat = u_anim_transform.m_transforms[i_joint_ids[0]] * i_joint_weights[0];

    for (uint i = 1; i < 4; ++i) {
        const int jid = i_joint_ids[i];
        if (-1 == jid)
            break;

        bone_mat += u_anim_transform.m_transforms[jid] * i_joint_weights[i];
    }

    return bone_mat;
}


void main() {
    const mat4 model_joint_mat = u_per_actor.m_model * make_joint_transform();
    const vec4 world_pos = model_joint_mat * vec4(i_position, 1);
    v_world_pos = world_pos.xyz;
    gl_Position = u_per_frame.m_proj * u_per_frame.m_view * world_pos;
    v_uv_coord = i_uv_coord;
    v_normal = normalize((model_joint_mat * vec4(i_normal, 0)).xyz);
}
