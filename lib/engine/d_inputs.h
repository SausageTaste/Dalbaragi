#pragma once

#include <array>
#include <bitset>

#include <glm/glm.hpp>


namespace dal {

    template <typename T, size_t S>
    class ResetQueue {

    private:
        std::array<T, S> m_data;
        size_t m_cur_index = 0;

    public:
        ResetQueue() = default;

    public:
        bool push_back(const T& data) {
            if (this->is_full())
                return false;

            this->m_data[this->m_cur_index] = data;
            this->m_cur_index++;
            return true;
        }

        void clear(void) {

        }

        bool is_full(void) const {
            return this->m_cur_index >= S;
        }

        size_t size(void) const {
            return this->m_cur_index;
        }

        constexpr
        size_t capacity(void) const {
            return S;
        }

        auto& at(const size_t index) const {
            return this->m_data.at(index);
        }

    };

}


namespace dal {

    struct MouseEvent;


    using touchID_t = std::int32_t;

    constexpr touchID_t NULL_TOUCH_ID = -1;

    enum class TouchActionType {
        cancel,
        down, move, up,
        hover_enter, hover_move, hover_exit,
    };

    struct TouchEvent {
        glm::vec2 m_pos;
        double m_time_sec;
        touchID_t m_id;
        TouchActionType m_action_type;

        TouchEvent() = default;
        TouchEvent(const MouseEvent& e);
    };


    enum class MouseActionType { down, move, up };

    enum class MouseButton { left, right };

    struct MouseEvent {
        glm::vec2 m_pos;
        double m_time_sec;
        MouseActionType m_action_type;
        MouseButton m_button;

        operator TouchEvent() const;
    };


    enum class KeyActionType { down, up };

    enum class KeyModifier { none, shift, ctrl, alt, caps_lock, num_lock };

    enum class KeyCode {
        unknown,
        a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z,
        n0, n1, n2, n3, n4, n5, n6, n7, n8, n9,  // Horizontal numbers
        backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash,
        space, enter, backspace, tab,
        escape, lshfit, rshfit, lctrl, rctrl, lalt, ralt, up, down, left, right,
        eoe
    };

    char encode_key_to_ascii(const dal::KeyCode key, const bool shift_pressed);


    class KeyEvent {

    private:
        using bitset_modifier_t = std::bitset<static_cast<int>(KeyModifier::num_lock)>;

    public:
        KeyCode m_key;
        KeyActionType m_action_type;
        double m_time_sec;

    private:
        bitset_modifier_t m_modifier;

    public:
        const bitset_modifier_t& modifier_states() const;
        bool modifier_state(const KeyModifier modifier_code) const;
        void set_modifier_state(const KeyModifier modifier_code, const bool pressed_state);
        void reset_modifier_states();

    private:
        bool are_no_mods_pressed() const;

    };

}


namespace dal {

    class TouchInputManager {

    private:
        ResetQueue<TouchEvent, 128> m_queue;

    public:
        bool push_back(const TouchEvent& e);

        auto& queue() {
            return this->m_queue;
        }
        auto& queue() const {
            return this->m_queue;
        }

    };


    class KeyInputManager {

    public:
        struct KeyState {
            double m_last_updated_sec = 0;
            bool m_pressed = false;
        };

    private:
        ResetQueue<KeyEvent, 128> m_queue;
        std::array<KeyState, static_cast<size_t>(KeyCode::eoe)> m_key_states;

    public:
        bool push_back(const KeyEvent& e);

        auto& queue() {
            return this->m_queue;
        }
        auto& queue() const {
            return this->m_queue;
        }

        auto& key_state_of(const KeyCode key_code) const {
            return this->m_key_states[static_cast<size_t>(key_code)];
        }

    };


    class InputManager {

    private:
        TouchInputManager m_touch_man;
        KeyInputManager m_key_man;

    public:
        auto& touch_manager() {
            return this->m_touch_man;
        }

        auto& key_manager() {
            return this->m_key_man;
        }

    };

}
