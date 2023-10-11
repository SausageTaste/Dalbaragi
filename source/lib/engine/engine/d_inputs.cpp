#include "d_inputs.h"

#include <fmt/format.h>

#include "dal/util/logger.h"


#define DAL_PRINT_INPUT false


namespace {

    constexpr float DEAD_ZONE_RATIO = 0.1;

    float calc_dead_zone_reduced(const float v, const float ratio = ::DEAD_ZONE_RATIO) {
        if (std::abs(v) < ratio)
            return 0;
        else
            return v;
    }

}


// TouchInputManager
namespace dal {

    bool TouchInputManager::push_back(const TouchEvent& e) {

#if DAL_PRINT_INPUT
        if (TouchActionType::move != e.m_action_type && TouchActionType::hover_move != e.m_action_type) {
            dalInfo(fmt::format(
                "Touch Event{{ type={}, id={}, time={}, pos={}x{} }}",
                static_cast<int>(e.m_action_type),
                e.m_id,
                e.m_time_sec,
                e.m_pos.x, e.m_pos.y
            ).c_str());
        }
#endif

        return this->m_queue.push_back(e);
    }

}


// KeyInputManager
namespace dal {

    bool KeyInputManager::push_back(const KeyEvent& e) {

#if DAL_PRINT_INPUT
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
                "Key Event{{ type={}, key={}, modifier={}, time={} }}",
                static_cast<int>(e.m_action_type),
                key_str,
                e.modifier_states().to_string(),
                e.m_time_sec
            ).c_str());
        }
#endif

        this->m_key_states.update_one(e);
        return this->m_queue.push_back(e);
    }

}


// GamepadState
namespace dal {

    void GamepadState::apply_dead_zone() {
        this->m_axis_left.x  = ::calc_dead_zone_reduced(this->m_axis_left.x);
        this->m_axis_left.y  = ::calc_dead_zone_reduced(this->m_axis_left.y);
        this->m_axis_right.x = ::calc_dead_zone_reduced(this->m_axis_right.x);
        this->m_axis_right.y = ::calc_dead_zone_reduced(this->m_axis_right.y);
    }

}


// GamepadInputManager
namespace dal {

    void GamepadInputManager::notify_connection_change(const GamepadConnectionEvent& e) {
        this->pad_list().erase(e.m_id);
        if (e.m_name.empty())
            return;

        auto [iter, success] = this->m_gamepads.emplace(e.m_id, GamepadState{});
        dalAssert(success);

        iter->second.m_name = e.m_name;

        dalInfo(fmt::format("Gamepad connected {{ id={}, name='{}' }}", e.m_id, e.m_name).c_str());
    }

    GamepadState& GamepadInputManager::get_gamepad_state(const int id) {
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

}
