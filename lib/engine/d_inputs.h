#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include <d_input_def.h>


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
            this->m_cur_index = 0;
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

        auto begin() {
            return this->m_data.begin();
        }

        auto begin() const {
            return this->m_data.begin();
        }

        auto end() {
            return this->m_data.begin() + this->size();
        }

        auto end() const {
            return this->m_data.begin() + this->size();
        }

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

        auto begin() {
            return this->m_queue.begin();
        }

        auto begin() const {
            return this->m_queue.begin();
        }

        auto end() {
            return this->m_queue.end();
        }

        auto end() const {
            return this->m_queue.end();
        }

    };


    class KeyInputManager {

    private:
        ResetQueue<KeyEvent, 128> m_queue;
        KeyStateRegistry m_key_states;

    public:
        bool push_back(const KeyEvent& e);

        auto& queue() {
            return this->m_queue;
        }

        auto& queue() const {
            return this->m_queue;
        }

        auto& key_state_of(const KeyCode key_code) const {
            return this->m_key_states[key_code];
        }

    };


    class GamepadInputManager {

    private:
        struct GamepadState {
            std::string m_name;

            glm::vec2 m_axis_left{ 0 };
            glm::vec2 m_axis_right{ 0 };
            glm::vec2 m_dpad{ 0 };
            float m_trigger_left = 0;
            float m_trigger_right = 0;
        };

    private:
        std::unordered_map<int, GamepadState> m_gamepads;
        using iterator_t = std::unordered_map<int, GamepadState>::iterator;

    public:
        void notify_connection_change(const GamepadConnectionEvent& e);

        GamepadState& get_gamepad_state(const int id);

        auto& pad_list() {
            return this->m_gamepads;
        }

        auto& pad_list() const {
            return this->m_gamepads;
        }

    };


    class InputManager {

    private:
        TouchInputManager m_touch_man;
        KeyInputManager m_key_man;
        GamepadInputManager m_gamepad_man;

    public:
        auto& touch_manager() {
            return this->m_touch_man;
        }

        auto& key_manager() {
            return this->m_key_man;
        }

        auto& gamepad_manager() {
            return this->m_gamepad_man;
        }

    };

}
