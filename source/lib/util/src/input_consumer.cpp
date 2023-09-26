#include "dal/util/input_consumer.h"


namespace dal {

    std::pair<InputConsumeResult, IInputListener*> InputDispatcher::dispatch(
        IInputListener* const* const begin,
        IInputListener* const* const end,
        const TouchEvent& e,
        const float w, const float h
    ) {
        auto& state = this->get_or_make_touch_state(e.m_id);

        if (nullptr != state.m_owner) {
            const auto ctrlFlag = state.m_owner->on_touch_event(e);

            switch (ctrlFlag) {
                case dal::InputConsumeResult::done_with:
                    state.m_owner = nullptr;
                    return { dal::InputConsumeResult::done_with, nullptr };
                case dal::InputConsumeResult::owned:
                    return { dal::InputConsumeResult::owned, state.m_owner };
                case dal::InputConsumeResult::ignored:
                    state.m_owner = nullptr;
                    break;  // Enters widgets loop below.
                default:
                    assert(false);
            }
        }

        for (auto x = begin ; end != x; ++x) {
            auto listener = *x;
            if (!listener->is_in_bound(e.m_pos.x, e.m_pos.y, w, h))
                continue;

            const auto ctrlFlag = listener->on_touch_event(e);

            switch (ctrlFlag) {
                case dal::InputConsumeResult::done_with:
                    return { dal::InputConsumeResult::done_with, listener };
                case dal::InputConsumeResult::ignored:
                    continue;
                case dal::InputConsumeResult::owned:
                    state.m_owner = listener;
                    return { dal::InputConsumeResult::owned, listener };
                default:
                    assert(false);
            }
        }

        return { dal::InputConsumeResult::ignored, nullptr };
    }

    void InputDispatcher::notify_listener_deleted(const IInputListener* const w) {
        for (auto& [id, state] : this->m_states) {
            if (w == state.m_owner)
                state.m_owner = nullptr;
        }
    }

    InputDispatcher::TouchState& InputDispatcher::get_or_make_touch_state(const dal::touchID_t id) {
        auto found = this->m_states.find(id);
        if (this->m_states.end() != found)
            return found->second;
        else
            return this->m_states.emplace(id, TouchState{}).first->second;
    }

}
