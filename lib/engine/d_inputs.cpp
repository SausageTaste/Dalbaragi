#include "d_inputs.h"

#include <fmt/format.h>

#include "d_logger.h"


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


namespace dal {

    bool TouchInputManager::push_back(const TouchEvent& e) {
        if (TouchActionType::move != e.m_action_type && TouchActionType::hover_move != e.m_action_type) {
            dalInfo(fmt::format(
                "Touch Event{{ type={}, id={}, time={}, pos={}x{} }}",
                static_cast<int>(e.m_action_type),
                e.m_id,
                e.m_time_sec,
                e.m_pos.x, e.m_pos.y
            ).c_str());
        }

        return this->m_queue.push_back(e);
    }

}

namespace dal {

    bool KeyInputManager::push_back(const KeyEvent& e) {
        // Print
        {
            std::string key_str;
            if (KeyCode::unknown == e.m_key) {
                key_str = "unkown";
            }
            else {
                const auto c = encode_key_to_ascii(e.m_key, e.modifier_state(KeyModifier::shift));
                key_str = ('\0' == c) ? std::to_string(static_cast<int>(e.m_key)) : fmt::format("\"{}\"", c);
            }

            dalInfo(fmt::format(
                "Touch Event{{ type={}, key={}, modifier={}, time={} }}",
                static_cast<int>(e.m_action_type),
                key_str,
                e.modifier_states().to_string(),
                e.m_time_sec
            ).c_str());
        }

        auto& state = this->m_key_states[static_cast<size_t>(e.m_key)];
        state.m_last_updated_sec = e.m_time_sec;

        switch (e.m_action_type) {
            case KeyActionType::down:
                state.m_pressed = true;
                break;
            case KeyActionType::up:
                state.m_pressed = false;
                break;
        }

        return this->m_queue.push_back(e);
    }

}


namespace dal {

    void GamepadInputManager::notify_connection_change(const GamepadConnectionEvent& e) {
        this->remove_gamepad(e.m_id);
        if (!e.m_connected)
            return;

        auto [iter, success] = this->m_gamepads.emplace(e.m_id, GamepadState{});
        dalAssert(success);

        iter->second.m_name = e.m_name;

        dalInfo(fmt::format("Gamepad connected {{ id={}, name='{}' }}", e.m_id, e.m_name).c_str());
    }

    GamepadInputManager::GamepadState& GamepadInputManager::get_gamepad_state(const int id) {
        auto found = this->m_gamepads.find(id);
        if (this->m_gamepads.end() == found) {
            auto [iter, success] = this->m_gamepads.emplace(id, GamepadState{});
            dalAssert(success);
            return iter->second;
        }
        else {
            return found->second;
        }
    }

    void GamepadInputManager::remove_gamepad(const int id) {
        auto iter = this->m_gamepads.find(id);
        if (this->m_gamepads.end() != iter) {
            dalInfo(fmt::format("Gamepad removed {{ id={}, name='{}' }}", iter->first, iter->second.m_name).c_str());
            this->m_gamepads.erase(iter);
        }
    }

}
