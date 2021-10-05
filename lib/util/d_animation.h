#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <daltools/struct.h>

#include "d_timer.h"


namespace dal {

    using jointID_t = dal::parser::jointID_t;

    using JointType = dal::parser::JointType;


    class JointSkel {

    private:
        std::string m_name;
        glm::mat4 m_joint_offset;
        glm::mat4 m_joint_offset_inv;
        glm::mat4 m_space_to_parent;
        jointID_t m_parent_index = -1;
        JointType m_joint_type = JointType::basic;

    public:
        void set(const dal::parser::SkelJoint& dst_joint);

        void set_parent_mat(const glm::mat4& mat) {
            this->m_space_to_parent = mat;
        }

        void set_parent_mat(const JointSkel& parent) {
            this->m_space_to_parent = parent.offset_inv() * this->offset();
        }

        const std::string& name() const {
            return this->m_name;
        }

        const glm::mat4& offset() const {
            return this->m_joint_offset;
        }

        const glm::mat4& offset_inv() const {
            return this->m_joint_offset_inv;
        }

        const glm::mat4& to_parent_mat() const {
            return this->m_space_to_parent;
        }

        jointID_t parent_index() const {
            return this->m_parent_index;
        }

        JointType join_type() const {
            return this->m_joint_type;
        }

        glm::vec3 local_pos() const;

    private:
        void set_offset_mat(const glm::mat4& mat);

    };


    class SkeletonInterface {

    private:
        std::map<std::string, jointID_t> m_name_joint_map;
        std::vector<JointSkel> m_joints;

    public:
        jointID_t get_index_of(const std::string& joint_mame) const;

        jointID_t get_or_make_index_of(const std::string& joint_mame);

        auto& at(const jointID_t index) {
            return this->m_joints.at(index);
        }

        auto& at(const jointID_t index) const {
            return this->m_joints.at(index);
        }

        jointID_t size() const {
            return static_cast<jointID_t>(this->m_joints.size());
        }

        bool is_empty() const {
            return this->m_joints.empty();
        }

        void clear() {
            this->m_name_joint_map.clear();
            this->m_joints.clear();
        }

    private:
        jointID_t upsize_and_get_index(void);

    };


    using TransformArray = std::vector<glm::mat4>;


    class IJointModifier {

    public:
        virtual ~IJointModifier(void) = default;
        virtual glm::mat4 makeTransform(const float deltaTime, const jointID_t jid, const dal::SkeletonInterface& skeleton) = 0;

    };

    using jointModifierRegistry_t = std::unordered_map<jointID_t, std::shared_ptr<IJointModifier>>;


    class JointAnim {

    public:
        dal::parser::AnimJoint m_data;

        glm::mat4 make_transform(const float anim_tick) const;

    private:
        bool has_key_frames() const;

        glm::vec3 interpolate_translate(const float anim_tick) const;

        glm::quat interpolate_rotation(const float anim_tick) const;

        float interpolate_scale(const float anim_tick) const;

    };


    class Animation {

    private:
        std::string m_name;
        std::vector<JointAnim> m_joints;
        float m_tick_per_sec;
        float m_duration_in_tick;

    public:
        Animation(const std::string& name, const float tick_per_sec, const float duration_tick);

        auto& new_joint() {
            return this->m_joints.emplace_back();
        }

        auto& name() const {
            return this->m_name;
        }

        float tick_per_sec() const {
            return this->m_tick_per_sec;
        }

        float duration_in_tick() const {
            return this->m_duration_in_tick;
        }

        void sample(
            const float elapsed,
            const float anim_tick,
            const SkeletonInterface& interf,
            TransformArray& trans_array,
            const jointModifierRegistry_t& modifiers
        ) const;

        float convert_sec_to_tick(const float seconds) const;

        Animation make_compatible_with(const dal::SkeletonInterface& skeleton) const;

    private:
        const JointAnim* find_by_name(const std::string& name) const;

    };


    class AnimationState {

    private:
        Timer m_local_timer;
        TransformArray m_final_transforms;
        jointModifierRegistry_t m_modifiers;
        size_t m_selected_anim_index = 0;
        double m_time_scale = 1;
        double m_local_time_accumulator = 0;

    public:
        double elapsed();

        auto& transform_array() {
            return this->m_final_transforms;
        }

        auto selected_anim_index() const {
            return this->m_selected_anim_index;
        }

        auto& joint_modifiers() const {
            return this->m_modifiers;
        }

        void set_anim_index(const size_t index);

        void set_time_scale(const double scale);

        void add_modifier(const jointID_t jid, std::shared_ptr<IJointModifier>& mod);

    };


    void update_anime_state(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf);

}
