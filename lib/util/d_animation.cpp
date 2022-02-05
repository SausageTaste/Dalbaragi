#include "d_animation.h"

#include <fmt/format.h>

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
    // WTF such a broken Engilsh that even I, the writer, can't understand!!
    template <typename T>
    size_t find_index_to_start_interp(const std::vector<std::pair<float, T>>& container, const float criteria) {
        dalAssert(0 != container.size());

        for (size_t i = 0; i < container.size() - 1; i++)
            if (criteria < container[i + 1].first)
                return i;

        return container.size() - 1;
    }

    template <typename T>
    T make_interp_value(const float anim_tick, const std::vector<std::pair<float, T>>& container) {
        dalAssert(0 != container.size());

        if ( 1 == container.size() ) {
            return container[0].second;
        }

        const auto start_index = find_index_to_start_interp(container, anim_tick);
        const auto next_index = start_index + 1;
        if (next_index >= container.size()) {
            return container.back().second;
        }

        const auto delta_time = container[next_index].first - container[start_index].first;
        auto factor = (anim_tick - container[start_index].first) / delta_time;
        if (0.f <= factor && factor <= 1.f) {
            const auto start = container[start_index].second;
            const auto end = container[next_index].second;
            return interpolate(start, end, factor);
        }
        else {
            const auto start = container[start_index].second;
            const auto end = container[next_index].second;
            return interpolate(start, end, 0.f);
        }
    }


    std::vector<dal::jointID_t> make_parent_list(const dal::jointID_t id, const dal::SkeletonInterface& interf) {
        std::vector<dal::jointID_t> result;

        dal::jointID_t current_id = id;
        while (true) {
            const auto parent_id = interf.at(current_id).parent_index();
            if (parent_id < 0) {
                return result;
            }
            else {
                result.push_back(parent_id);
                current_id = parent_id;
            }
        }
    }

    std::pair<glm::vec3, glm::quat> decompose_transform(const glm::mat4& mat) {
        return std::make_pair(glm::vec3(mat[3]), glm::quat_cast(mat));
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
        const auto result = decompose_transform(this->offset());
        return result.first;
    }

    void JointSkel::set_offset_mat(const glm::mat4& mat) {
        this->m_joint_offset = mat;
        this->m_joint_offset_inv = glm::inverse(this->m_joint_offset);
    }

}


// SkeletonInterface
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

}


// JointAnim
namespace dal {

    glm::mat4 JointAnim::make_transform(const float anim_tick) const {
        const auto translate = this->interpolate_translate(anim_tick);
        const auto rotate = this->interpolate_rotation(anim_tick);
        const auto scale = this->interpolate_scale(anim_tick);

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


// Animation
namespace dal {

    Animation::Animation(const std::string& name, const float tick_per_sec, const float duration_tick)
        : m_name(name)
        , m_tick_per_sec(tick_per_sec)
        , m_duration_in_tick(duration_tick)
    {
        if (0.0f == this->m_duration_in_tick)
            this->m_duration_in_tick = 1;

        if (0.0f == this->m_tick_per_sec)
            this->m_tick_per_sec = 25;
    }

    void Animation::sample(
        const float elapsed,
        const float anim_tick,
        const SkeletonInterface& interf,
        TransformArray& trans_array,
        const jointModifierRegistry_t& modifiers
    ) const {
        static const auto SPACE_ANIM_TO_MODEL = glm::rotate(glm::mat4{ 1.f }, glm::radians(-90.f), glm::vec3{ 1.f, 0.f, 0.f });
        static const auto SPACE_MODEL_TO_ANIM = glm::inverse(SPACE_ANIM_TO_MODEL);

        const auto num_joints = interf.size();
        dalAssert(num_joints == this->m_joints.size());

        trans_array.resize(num_joints);
        std::vector<glm::mat4> joint_transforms(num_joints);

        for (int i = 0; i < num_joints; ++i) {
            const auto found = modifiers.find(i);
            if (modifiers.end() != found)
                joint_transforms[i] = found->second->makeTransform(elapsed, i, interf);
            else
                joint_transforms[i] = this->m_joints[i].make_transform(anim_tick);
        }

        for (int i = 0; i < num_joints; ++i) {
            dal::jointID_t cur_jid = i;
            glm::mat4 total_trans = interf.at(i).offset_inv();

            while (-1 != cur_jid) {
                total_trans = interf.at(cur_jid).to_parent_mat() * joint_transforms[cur_jid] * total_trans;
                cur_jid = interf.at(cur_jid).parent_index();
            }

            trans_array.at(i) = SPACE_ANIM_TO_MODEL * total_trans * SPACE_MODEL_TO_ANIM;
        }

        return;
    }

    float Animation::convert_sec_to_tick(const float seconds) const {
        const auto anim_duration = this->duration_in_tick();
        const auto time_in_ticks = seconds * this->tick_per_sec();
        return std::fmod(time_in_ticks, anim_duration);
    }

    Animation Animation::make_compatible_with(const dal::SkeletonInterface& skeleton) const {
        Animation output{ this->name(), this->tick_per_sec(), this->duration_in_tick() };
        output.m_joints.resize(skeleton.size());

        for (dal::jointID_t i = 0; i < skeleton.size(); ++i) {
            const auto src_joint = this->find_by_name(skeleton.at(i).name());
            if (nullptr != src_joint) {
                output.m_joints[i] = *src_joint;
            }
            else {
                output.m_joints[i].m_data.m_name = skeleton.at(i).name();
            }
        }

        return output;
    }

    // Private

    const JointAnim* Animation::find_by_name(const std::string& name) const {
        for (auto& joint : this->m_joints) {
            if (joint.m_data.m_name == name) {
                return &joint;
            }
        }

        return nullptr;
    }

}


namespace dal {

    double AnimationState::elapsed() {
        const auto delta_time = this->m_local_timer.check_get_elapsed();
        this->m_local_time_accumulator += delta_time * this->m_time_scale;
        return this->m_local_time_accumulator;
    }

    void AnimationState::set_anim_index(const size_t index) {
        if (index != this->m_selected_anim_index) {
            this->m_selected_anim_index = index;
            this->m_local_time_accumulator = 0;
        }
    }

    void AnimationState::set_time_scale(const double scale) {
        this->m_time_scale = scale;
    }

    void AnimationState::add_modifier(const jointID_t jid, std::shared_ptr<IJointModifier>& mod) {
        this->m_modifiers.emplace(jid, std::move(mod));
    }

}


// Functions
namespace dal {

    void update_anime_state(AnimationState& state, const std::vector<Animation>& anims, const SkeletonInterface& skeletonInterf) {
        const auto selected_anim_index = state.selected_anim_index();
        if (selected_anim_index >= anims.size()) {
            //dalError(fmt::format("Selected animation's index is out of range: {}", selected_anim_index).c_str());
            return;
        }

        const auto& anim = anims[selected_anim_index];
        const auto elapsed = static_cast<float>(state.elapsed());
        const auto anim_tick = anim.convert_sec_to_tick(elapsed);
        anim.sample(elapsed, anim_tick, skeletonInterf, state.transform_array(), state.joint_modifiers());
    }

}
