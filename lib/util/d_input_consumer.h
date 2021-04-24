#pragma once

#include <unordered_map>

#include "d_input_def.h"


namespace dal {

    enum class InputConsumeResult { ignored, done_with, owned };


    class IInputListener {

    public:
        virtual ~IInputListener() = default;

        virtual InputConsumeResult on_touch_event(const TouchEvent& e) {
            return InputConsumeResult::ignored;
        }

        virtual InputConsumeResult on_key_event(const KeyEvent& e, const KeyStateRegistry& registry) {
            return InputConsumeResult::ignored;
        }

        // The point is in screen space, where x is in [0, w] and y is in [0, h]
        virtual bool is_in_bound(const float x, const float y, const float w, const float h) { return true; };

    };


    class InputDispatcher {

    private:
        struct TouchState {
            IInputListener* m_owner = nullptr;
        };

    private:
        using states_t = std::unordered_map<touchID_t, TouchState>;
        states_t m_states;

    public:
        // Returns a pointer to a listener that owns the touch id.
        // Returns nullptr if none of listeners owns it.
        std::pair<InputConsumeResult, IInputListener*> dispatch(
            IInputListener* const* const begin,
            IInputListener* const* const end,
            const TouchEvent& e,
            const float w, const float h
        );

        void notify_listener_deleted(const IInputListener* const w);

    private:
        TouchState& get_or_make_touch_state(const dal::touchID_t id);

    };

}
