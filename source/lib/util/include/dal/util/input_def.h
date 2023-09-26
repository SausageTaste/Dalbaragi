#pragma once

#include <array>
#include <bitset>

#include <glm/glm.hpp>


// Touch
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

}


// Mouse
namespace dal {

    enum class MouseActionType { down, move, up };

    enum class MouseButton { left, right };

    struct MouseEvent {
        glm::vec2 m_pos;
        double m_time_sec;
        MouseActionType m_action_type;
        MouseButton m_button;

        operator TouchEvent() const;
    };

}


// Keyboard
namespace dal {

    enum class KeyActionType { down, up, repeat };

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


    class KeyStateRegistry {

    public:
        struct KeyState {
            double m_last_updated_sec = 0;
            bool m_pressed = false;
        };

    private:
        static constexpr auto KEY_CODE_SIZE = static_cast<unsigned int>(KeyCode::eoe) - static_cast<unsigned int>(KeyCode::unknown);
        std::array<KeyState, KEY_CODE_SIZE> m_states;

    public:
        void update_one(const KeyEvent& e) {
            const auto index = this->to_index(e.m_key);
            this->m_states[index].m_last_updated_sec = e.m_time_sec;
            this->m_states[index].m_pressed = (e.m_action_type != KeyActionType::up);
        }

        KeyState& operator[](const KeyCode key) {
            return this->m_states[this->to_index(key)];
        }

        const KeyState& operator[](const KeyCode key) const {
            return this->m_states[this->to_index(key)];
        }

    private:
        static size_t to_index(const KeyCode key) {
            return static_cast<size_t>(key) - static_cast<size_t>(KeyCode::unknown);
        }

    };

}


// Game pad
namespace dal {

    struct GamepadConnectionEvent {
        std::string m_name;
        int m_id;
        bool m_connected;
    };

}
