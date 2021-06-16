#include "d_animation.h"

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>

#include <d_logger.h>


namespace {

    float interpolate(const float start, const float end, const float factor) {
        const auto delta = end - start;
        return start + factor * delta;
    }

    glm::vec3 interpolate(const glm::vec3& start, const glm::vec3& end, const float factor) {
        const auto delta = end - start;
        return start + factor * delta;
    }

    glm::quat interpolate(const glm::quat& start, const glm::quat& end, const float factor) {
        return glm::slerp(start, end, factor);
    }

    // Pass it container with size 0 and iterator to index of unsigned{ -1 }.
    // WTF such a broken Engilsh that even I can't understand!!
    template <typename T>
    size_t find_index_to_start_interp(const std::vector<std::pair<float, T>>& container, const float criteria) {
        dalAssert(0 != container.size());

        for ( size_t i = 0; i < container.size() - 1; i++ ) {
            if ( criteria < container[i + 1].first ) {
                return i;
            }
        }

        return container.size() - 1;
    }

    template <typename T>
    T make_interp_value(const float animTick, const std::vector<std::pair<float, T>>& container) {
        dalAssert(0 != container.size());

        if ( 1 == container.size() ) {
            return container[0].second;
        }

        const auto startIndex = find_index_to_start_interp(container, animTick);
        const auto nextIndex = startIndex + 1;
        if ( nextIndex >= container.size() ) {
            return container.back().second;
        }

        const auto deltaTime = container[nextIndex].first - container[startIndex].first;
        auto factor = (animTick - container[startIndex].first) / deltaTime;
        if ( 0.0f <= factor && factor <= 1.0f ) {
            const auto start = container[startIndex].second;
            const auto end = container[nextIndex].second;
            return interpolate(start, end, factor);
        }
        else {
            const auto start = container[startIndex].second;
            const auto end = container[nextIndex].second;
            return interpolate(start, end, 0.0f);
        }
    }

    std::vector<dal::jointID_t> makeParentList(const dal::jointID_t id, const dal::SkeletonInterface& interf) {
        std::vector<dal::jointID_t> result;

        dal::jointID_t currentID = id;
        while ( true ) {
            const auto parentID = interf.at(currentID).parent_index();
            if ( parentID < 0 ) {
                return result;
            }
            else {
                result.push_back(parentID);
                currentID = parentID;
            }
        }
    }

    std::pair<glm::vec3, glm::quat> decomposeTransform(const glm::mat4& mat) {
        return std::pair<glm::vec3, glm::quat>{
            glm::vec3(mat[3]),
                glm::quat_cast(mat)
        };
    }

}


// JointSkel
namespace dal {

    void JointSkel::set(const dal::parser::SkelJoint& src_joint) {
        this->m_name = src_joint.m_name;

        this->m_parent_index = src_joint.m_parent_index;
        this->set_offset_mat(src_joint.m_offset_mat);
        this->m_joint_type = src_joint.m_joint_type;
    }

    glm::vec3 JointSkel::local_pos(void) const {
        const auto result = decomposeTransform(this->offset());
        return result.first;
    }

    void JointSkel::set_offset_mat(const glm::mat4& mat) {
        this->m_joint_offset = mat;
        this->m_joint_offset_inv = glm::inverse(this->m_joint_offset);
    }

}


namespace dal {

    jointID_t SkeletonInterface::get_index_of(const std::string& joint_mame) const {
        auto iter = this->m_name_joint_map.find(joint_mame);
        return this->m_name_joint_map.end() != iter ? iter->second : -1;
    }

    jointID_t SkeletonInterface::get_or_make_index_of(const std::string& joint_mame) {
        const auto index = this->get_index_of(joint_mame);

        if (-1 == index) {
            const auto new_index = this->upsize_and_get_index();
            this->m_name_joint_map.emplace(joint_mame, new_index);
            return new_index;
        }
        else {
            return index;
        }
    }

    // Private

    jointID_t SkeletonInterface::upsize_and_get_index(void) {
        this->m_joints.emplace_back();
        return this->m_joints.size() - 1;
    }

}  // namespace dal


// JointAnim
namespace dal {

    glm::mat4 JointAnim::make_transform(const float animTick) const {
        const auto translate = this->interpolate_translate(animTick);
        const auto rotate = this->interpolate_rotation(animTick);
        const auto scale = this->interpolate_scale(animTick);

        const glm::mat4 identity{1};
        const auto translate_mat = glm::translate(identity, translate);
        const auto rotate_mat = glm::mat4_cast(rotate);
        const auto scale_mat = glm::scale(identity, glm::vec3{ scale });

        return translate_mat * rotate_mat * scale_mat;
    }

    // Private

    bool JointAnim::has_key_frames() const {
        return (this->m_data.m_translates.size() + this->m_data.m_rotations.size() + this->m_data.m_scales.size()) != 0;
    }

    glm::vec3 JointAnim::interpolate_translate(const float anim_tick) const {
        return this->m_data.m_translates.empty() ? glm::vec3{0} : ::make_interp_value(anim_tick, this->m_data.m_translates);
    }

    glm::quat JointAnim::interpolate_rotation(const float anim_tick) const {
        return this->m_data.m_rotations.empty() ? glm::quat{} : ::make_interp_value(anim_tick, this->m_data.m_rotations);
    }

    float JointAnim::interpolate_scale(const float anim_tick) const {
        return this->m_data.m_scales.empty() ? 1 : ::make_interp_value(anim_tick, this->m_data.m_scales);
    }

}


namespace dal {

    Animation::Animation(const std::string& name, const float tickPerSec, const float durationTick)
        : m_name(name)
        , m_tickPerSec(tickPerSec)
        , m_durationInTick(durationTick)
    {
        if ( 0.0f == this->m_durationInTick ) {
            this->m_durationInTick = 1.0f;
        }
        if ( 0.0f == this->m_tickPerSec ) {
            this->m_tickPerSec = 25.0f;
        }
    }

    void Animation::sample2(const float elapsed, const float animTick, const SkeletonInterface& interf,
        TransformArray& transformArr, const jointModifierRegistry_t& modifiers) const {
        static const auto k_spaceAnim2Model = glm::rotate(glm::mat4{ 1.f }, glm::radians(-90.f), glm::vec3{ 1.f, 0.f, 0.f });
        static const auto k_spaceModel2Anim = glm::inverse(k_spaceAnim2Model);

        const auto numBones = interf.size();
        transformArr.resize(numBones);

        std::vector<glm::mat4> boneTransforms(numBones);
        dalAssert(numBones == this->m_joints.size());
        for ( int i = 0; i < numBones; ++i ) {
            const auto found = modifiers.find(i);
            if ( modifiers.end() != found ) {
                boneTransforms[i] = found->second->makeTransform(elapsed, i, interf);
            }
            else {
                boneTransforms[i] = this->m_joints[i].make_transform(animTick);
            }
        }

        for ( dal::jointID_t i = 0; i < numBones; ++i ) {
            dal::jointID_t curBone = i;
            glm::mat4 totalTrans = interf.at(i).offset_inv();
            while ( curBone != -1 ) {
                totalTrans = interf.at(curBone).to_parent_mat() * boneTransforms[curBone] * totalTrans;
                curBone = interf.at(curBone).parent_index();
            }
            transformArr.at(i) = k_spaceAnim2Model * totalTrans * k_spaceModel2Anim;
        }

        return;
    }

    float Animation::calcAnimTick(const float seconds) const {
        const auto animDuration = this->getDurationInTick();
        const auto animTickPerSec = this->getTickPerSec();
        float TimeInTicks = seconds * animTickPerSec;
        return fmod(TimeInTicks, animDuration);
    }

}  // namespace dal


namespace dal {

    float AnimationState::getElapsed(void) {
        const auto deltaTime = this->m_localTimer.check_get_elapsed();
        this->m_localTimeAccumulator += deltaTime * this->m_timeScale;
        return this->m_localTimeAccumulator;
    }

    TransformArray& AnimationState::getTransformArray(void) {
        return this->m_finalTransform;
    }

    unsigned int AnimationState::getSelectedAnimeIndex(void) const {
        return this->m_selectedAnimIndex;
    }

    void AnimationState::setSelectedAnimeIndex(const unsigned int index) {
        if ( this->m_selectedAnimIndex != index ) {
            this->m_selectedAnimIndex = index;
            this->m_localTimeAccumulator = 0.0f;
        }
    }

    void AnimationState::setTimeScale(const float scale) {
        this->m_timeScale = scale;
    }

    void AnimationState::addModifier(const jointID_t jid, std::shared_ptr<IJointModifier> mod) {
        this->m_modifiers.emplace(jid, std::move(mod));
    }

}


// Functions
namespace dal {

    void updateAnimeState(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf) {
        const auto selectedAnimIndex = state.getSelectedAnimeIndex();
        if ( selectedAnimIndex >= anims.size() ) {
            //dalError(fmt::format("Selected animation's index is out of range: {}", selectedAnimIndex).c_str());
            return;
        }

        const auto& anim = anims[selectedAnimIndex];
        const auto elapsed = state.getElapsed();
        const auto animTick = anim.calcAnimTick(elapsed);
        anim.sample2(elapsed, animTick, skeletonInterf, state.getTransformArray(), state.getModifiers());
    }

}
