#pragma once

#include <vector>
#include <functional>


namespace dal {

    class LoggerSingleton {

    public:
        using print_func_t = std::function<void(const char*)>;

    private:
        std::vector<print_func_t> m_print_funcs;

    private:
        LoggerSingleton() = default;

    public:
        static auto& inst() {
            static LoggerSingleton inst;
            return inst;
        }

        void add_print_func(const print_func_t& func) {
            this->m_print_funcs.push_back(func);
        }

        void simple_print(const char* const text) const {
            for (auto& x : this->m_print_funcs) {
                x(text);
            }
        }

    };

}
