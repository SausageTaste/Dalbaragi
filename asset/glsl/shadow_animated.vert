#version 450


layout(location = 0) in ivec4 i_joint_ids;
layout(location = 1) in vec4 i_joint_weights;
layout(location = 2) in vec3 i_position;
layout(location = 3) in vec3 i_normal;
layout(location = 4) in vec2 i_uv_coord;


layout(push_constant) uniform PushConstant {
    mat4 m_model_mat;
    mat4 m_light_mat;
} u_pc;

layout(set = 0, binding = 0) uniform U_AnimTransform {
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
    const mat4 model_joint_mat = u_pc.m_model_mat * make_joint_transform();
    gl_Position = u_pc.m_light_mat * model_joint_mat * vec4(i_position, 1.0);
}
