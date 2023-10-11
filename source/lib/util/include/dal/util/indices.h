#pragma once

#include <limits>
#include <cstdint>

#include "dal/util/konsts.h"


namespace dal {

    template <typename T>
    class IIndex {

    private:
        T m_value = 0;

    public:
        IIndex()
            : m_value(0)
        {

        }

        explicit
        IIndex(const T v) noexcept
            : m_value(v)
        {

        }

        IIndex& operator=(const T v) {
            this->set(v);
            return *this;
        }

        T& operator*() noexcept {
            return this->m_value;
        }

        const T& operator*() const noexcept {
            return this->m_value;
        }

        const T& get() const noexcept {
            return this->m_value;
        }

        void set(const T v) noexcept {
            this->m_value = v;
        }

        void set_max_value() noexcept {
            this->m_value = (std::numeric_limits<T>::max)();
        }

    };


    class FrameInFlightIndex : public IIndex<uint32_t> {

    public:
        using IIndex::IIndex;
        using IIndex::operator=;

        void increase() noexcept {
            this->set( (this->get() + 1) % MAX_FRAMES_IN_FLIGHT );
        }

    };


    class SwapchainIndex : public IIndex<uint32_t> {

    public:
        using IIndex::IIndex;
        using IIndex::operator=;

        [[nodiscard]]
        static SwapchainIndex max_value() noexcept {
            SwapchainIndex output;
            output.set_max_value();
            return output;
        }

    };

}
