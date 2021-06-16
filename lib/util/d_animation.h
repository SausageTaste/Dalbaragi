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

#include "d_timer.h"
#include "dal_struct.h"


namespace dal {

    using jointID_t = int32_t;

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
        float m_tickPerSec, m_durationInTick;

    public:
        Animation(const Animation&) = default;
        Animation& operator=(const Animation&) = default;
        Animation(Animation&&) = default;
        Animation& operator=(Animation&&) = default;

    public:
        Animation(const std::string& name, const float tickPerSec, const float durationTick);

        JointAnim& newJoint(void) {
            return this->m_joints.emplace_back();
        }

        const std::string& getName(void) const {
            return this->m_name;
        }
        float getTickPerSec(void) const {
            return this->m_tickPerSec;
        }
        float getDurationInTick(void) const {
            return this->m_durationInTick;
        }

        void sample2(const float elapsed, const float animTick, const SkeletonInterface& interf,
            TransformArray& transformArr, const jointModifierRegistry_t& modifiers) const;
        float calcAnimTick(const float seconds) const;

    };


    class AnimationState {

    private:
        Timer m_localTimer;
        TransformArray m_finalTransform;
        jointModifierRegistry_t m_modifiers;
        unsigned int m_selectedAnimIndex = 0;
        float m_timeScale = 1.0f;
        float m_localTimeAccumulator = 0.0f;

    public:
        float getElapsed(void);
        TransformArray& getTransformArray(void);
        unsigned int getSelectedAnimeIndex(void) const;

        void setSelectedAnimeIndex(const unsigned int index);
        void setTimeScale(const float scale);

        void addModifier(const jointID_t jid, std::shared_ptr<IJointModifier> mod);
        const jointModifierRegistry_t& getModifiers(void) const {
            return this->m_modifiers;
        }

    };


    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf);

}
