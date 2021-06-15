#version 450


layout(location = 0) in ivec4 i_joint_ids;
layout(location = 1) in vec4 i_joint_weights;
layout(location = 2) in vec3 i_position;
layout(location = 3) in vec3 i_normal;
layout(location = 4) in vec2 i_uv_coord;

layout(location = 0) out vec2 v_uv_coord;
layout(location = 1) out vec3 v_normal;


layout(set = 0, binding = 0) uniform U_PerFrame {
    mat4 m_view;
    mat4 m_proj;
    vec4 m_view_pos;
} u_per_frame;

layout(set = 2, binding = 0) uniform U_PerActor {
    mat4 m_model;
} u_per_actor;

layout(set=3, binding=0) uniform U_AnimTransform {
    mat4 m_transforms[128];
} u_anim_transform;


mat4 make_joint_transform(ivec4 joint_ids, vec4 weights) {
    if (-1 == joint_ids[0])
        return mat4();

    mat4 bone_mat = u_anim_transform.m_transforms[joint_ids[0]] * weights[0];

    for (uint i = 1; i < 4; ++i) {
        int jid = joint_ids[i];
        if (-1 == jid)
            break;

        bone_mat += u_anim_transform.m_transforms[jid] * weights[i];
    }

    return bone_mat;
}


void main() {
    const mat4 model_joint_mat = u_per_actor.m_model * make_joint_transform(i_joint_ids, i_joint_weights);
    gl_Position = u_per_frame.m_proj * u_per_frame.m_view * model_joint_mat * vec4(i_position, 1);
    v_uv_coord = i_uv_coord;
    v_normal = (u_per_actor.m_model * vec4(i_normal, 0)).xyz;
}
