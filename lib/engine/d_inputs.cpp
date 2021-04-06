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
