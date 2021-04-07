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

    void TouchInputManager::clear() {
        this->m_queue.clear();
    }

}

namespace dal {

    bool KeyInputManager::push_back(const KeyEvent& e) {
        std::string key_str;
        if (KeyCode::unknown == e.m_key) {
            key_str = "unkown";
        }
        else {
            const auto c = encode_key_to_ascii(e.m_key, false);
            key_str = ('\0' == c) ? std::to_string(static_cast<int>(e.m_key)) : fmt::format("\"{}\"", c);
        }

        dalInfo(fmt::format(
            "Touch Event{{ type={}, key={}, time={} }}",
            static_cast<int>(e.m_action_type),
            key_str,
            e.m_time_sec
        ).c_str());

        return this->m_queue.push_back(e);
    }

    void KeyInputManager::clear() {
        this->m_queue.clear();
    }

}
