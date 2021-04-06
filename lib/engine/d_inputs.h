#pragma once

#include <array>

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

}


namespace dal {

    class TouchInputManager {

    private:
        ResetQueue<TouchEvent, 128> m_queue;

    public:
        bool push_back(const TouchEvent& e);

        void clear();

    };


    class InputManager {

    private:
        TouchInputManager m_touch_man;

    public:
        auto& touch_manager() {
            return this->m_touch_man;
        }

    };

}
