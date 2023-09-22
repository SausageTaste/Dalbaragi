#include "d_input_def.h"


namespace dal {

    TouchEvent::TouchEvent(const MouseEvent& e) {
        this->m_id       = 0;
        this->m_pos      = e.m_pos;
        this->m_time_sec = e.m_time_sec;

        switch (e.m_action_type) {
            case dal::MouseActionType::down:
                this->m_action_type = dal::TouchActionType::down;
                break;
            case dal::MouseActionType::move:
                this->m_action_type = dal::TouchActionType::move;
                break;
            case dal::MouseActionType::up:
                this->m_action_type = dal::TouchActionType::up;
                break;
            default:
                this->m_action_type = dal::TouchActionType::cancel;
        }
    }

    char encode_key_to_ascii(const dal::KeyCode key, const bool shift_pressed) {
        const auto keyInt = static_cast<int>(key);

        if ( static_cast<int>(dal::KeyCode::a) <= keyInt && keyInt <= static_cast<int>(dal::KeyCode::z) ) {
            if (shift_pressed) {
                return static_cast<char>(static_cast<int>('A') + keyInt - static_cast<int>(dal::KeyCode::a));
            }
            else {
                return char(static_cast<int>('a') + keyInt - static_cast<int>(dal::KeyCode::a));
            }
        }
        else if ( static_cast<int>(dal::KeyCode::n0) <= keyInt && keyInt <= static_cast<int>(dal::KeyCode::n9) ) {
            if (shift_pressed) {
                const auto index = keyInt - static_cast<int>(dal::KeyCode::n0);
                constexpr char map[] = { ')','!','@','#','$','%','^','&','*','(' };
                return map[index];
            }
            else {
                return static_cast<char>(static_cast<int>('0') + keyInt - static_cast<int>(dal::KeyCode::n0));
            }
        }
        else if ( static_cast<int>(dal::KeyCode::backquote) <= keyInt && keyInt <= static_cast<int>(dal::KeyCode::slash) ) {
            // backquote, minus, equal, lbracket, rbracket, backslash, semicolon, quote, comma, period, slash
            const auto index = keyInt - static_cast<int>(dal::KeyCode::backquote);
            if (shift_pressed) {
                constexpr char map[] = { '~', '_', '+', '{', '}', '|', ':', '"', '<', '>', '?' };
                return map[index];
            }
            else {
                constexpr char map[] = { '`', '-', '=', '[', ']', '\\', ';', '\'', ',', '.', '/' };
                return map[index];
            }
        }
        else if ( static_cast<int>(dal::KeyCode::space) <= keyInt && keyInt <= static_cast<int>(dal::KeyCode::tab) ) {
            // space, enter, backspace, tab
            const auto index = keyInt - static_cast<int>(dal::KeyCode::space);
            constexpr char map[] = { ' ', '\n', '\b', '\t' };
            return map[index];
        }
        else {
            return '\0';
        }
    }

}


// KeyEvent
namespace dal {

    const KeyEvent::bitset_modifier_t& KeyEvent::modifier_states() const {
        return this->m_modifier;
    }

    bool KeyEvent::modifier_state(const KeyModifier modifier_code) const {
        if (KeyModifier::none == modifier_code)
            return this->are_no_mods_pressed();
        else
            return true == this->m_modifier[static_cast<int>(modifier_code) - 1];
    }

    void KeyEvent::set_modifier_state(const KeyModifier modifier_code, const bool pressed_state) {
        if (KeyModifier::none == modifier_code)
            return;
        else
            this->m_modifier[static_cast<int>(modifier_code) - 1] = pressed_state;
    }

    void KeyEvent::reset_modifier_states() {
        this->m_modifier.reset();
    }

    bool KeyEvent::are_no_mods_pressed() const {
        return 0 == this->m_modifier.count();
    }

}
