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
        std::map<std::string, jointID_t> m_map;
        std::vector<JointSkel> m_boneInfo;
        jointID_t m_lastMadeIndex = -1;

    public:
        SkeletonInterface(const SkeletonInterface&) = default;
        SkeletonInterface& operator=(const SkeletonInterface&) = default;
        SkeletonInterface(SkeletonInterface&&) = default;
        SkeletonInterface& operator=(SkeletonInterface&&) = default;

        SkeletonInterface(void) = default;

    public:
        jointID_t getIndexOf(const std::string& jointName) const;
        jointID_t getOrMakeIndexOf(const std::string& jointName);

        JointSkel& at(const jointID_t index);
        const JointSkel& at(const jointID_t index) const;

        jointID_t getSize(void) const;
        bool isEmpty(void) const;

        void clear(void);

    private:
        jointID_t upsizeAndGetIndex(void);
        bool isIndexValid(const jointID_t index) const;

    };


    class JointTransformArray {

    private:
        std::vector<glm::mat4> m_array;

    public:
        JointTransformArray(const JointTransformArray&) = delete;
        JointTransformArray& operator=(const JointTransformArray&) = delete;
        JointTransformArray(JointTransformArray&&) = default;
        JointTransformArray& operator=(JointTransformArray&&) = default;

        JointTransformArray(void) = default;

    public:
        void setSize(const jointID_t size);
        void setTransform(const jointID_t index, const glm::mat4& mat);

        jointID_t getSize(void) const {
            return this->m_array.size();
        }

        auto& list() const {
            return this->m_array;
        }

    };


    class IJointModifier {

    public:
        virtual ~IJointModifier(void) = default;
        virtual glm::mat4 makeTransform(const float deltaTime, const jointID_t jid, const dal::SkeletonInterface& skeleton) = 0;

    };

    using jointModifierRegistry_t = std::unordered_map<jointID_t, std::shared_ptr<IJointModifier>>;


    class Animation {

    public:
        class JointNode {

        private:
            std::string m_name;

            std::vector<std::pair<float, glm::vec3>> m_poses;
            std::vector<std::pair<float, glm::quat>> m_rotates;
            std::vector<std::pair<float, float>> m_scales;

        public:
            JointNode(const JointNode&) = default;
            JointNode& operator=(const JointNode&) = default;
            JointNode(JointNode&&) = default;
            JointNode& operator=(JointNode&&) = default;

        public:
            JointNode(void) = default;;

            void setName(const std::string& name) {
                this->m_name = name;
            }
            void addPos(const float timepoint, const glm::vec3& pos) {
                const std::pair<float, glm::vec3> input{ timepoint, pos };
                this->m_poses.push_back(input);
            }
            void addRotation(const float timepoint, const glm::quat& rot) {
                const std::pair<float, glm::quat> input{ timepoint, rot };
                this->m_rotates.push_back(input);
            }
            void addScale(const float timepoint, const float scale) {
                const std::pair<float, float> input{ timepoint, scale };
                this->m_scales.push_back(input);
            }

            const std::string& name(void) const {
                return this->m_name;
            }

            glm::mat4 makeTransform(const float animTick) const;

        private:
            bool hasKeyframes(void) const;

            glm::vec3 makePosInterp(const float animTick) const;
            glm::quat makeRotateInterp(const float animTick) const;
            float makeScaleInterp(const float animTick) const;

        };

    private:
        std::string m_name;
        std::vector<JointNode> m_joints;
        float m_tickPerSec, m_durationInTick;

    public:
        Animation(const Animation&) = default;
        Animation& operator=(const Animation&) = default;
        Animation(Animation&&) = default;
        Animation& operator=(Animation&&) = default;

    public:
        Animation(const std::string& name, const float tickPerSec, const float durationTick);

        JointNode& newJoint(void) {
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
            JointTransformArray& transformArr, const jointModifierRegistry_t& modifiers) const;
        float calcAnimTick(const float seconds) const;

    };


    class AnimationState {

    private:
        Timer m_localTimer;
        JointTransformArray m_finalTransform;
        jointModifierRegistry_t m_modifiers;
        unsigned int m_selectedAnimIndex = 0;
        float m_timeScale = 1.0f;
        float m_localTimeAccumulator = 0.0f;

    public:
        float getElapsed(void);
        JointTransformArray& getTransformArray(void);
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
