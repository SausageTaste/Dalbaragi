#version 450


layout(location = 0) in ivec4 i_joint_ids;
layout(location = 1) in vec4 i_joint_weights;
layout(location = 2) in vec3 i_position;
layout(location = 3) in vec3 i_normal;
layout(location = 4) in vec2 i_uv_coord;

layout(location = 0) out vec3 v_world_pos;
layout(location = 1) out vec2 v_uv_coord;
layout(location = 2) out vec3 v_normal;
layout(location = 3) out vec3 v_light;


layout(set = 1, binding = 0) uniform U_PerActor {
    mat4 m_model;
} u_actor;

layout(set = 1, binding = 1) uniform U_AnimTransform {
    mat4 m_transforms[256];
} u_anim_transform;


layout(push_constant) uniform U_PC_OnMirror {
    mat4 m_proj_view_mat;
    vec4 m_clip_plane;
} u_pc;


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
    const mat4 model_joint_mat = u_actor.m_model * make_joint_transform();
    const vec4 world_pos = model_joint_mat * vec4(i_position, 1);

    gl_Position = u_pc.m_proj_view_mat * world_pos;
    v_world_pos = world_pos.xyz;
    v_uv_coord = i_uv_coord;
    v_normal = normalize(mat3(model_joint_mat) * i_normal);
    v_light = vec3(max(0, dot(v_normal, normalize(vec3(1, 1, 0)))) * 0.2 + 0.1);
}
