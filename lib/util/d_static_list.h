#pragma once

#include <array>


namespace dal {

    template <size_t _BlockSize, size_t _ElementCount>
    class ObjectSizedBytesArray {

    private:
        uint8_t m_arr[_BlockSize * _ElementCount];

    public:
        void* operator[](const size_t index) {
            const auto arr_index = index * _BlockSize;
            return reinterpret_cast<void*>(this->m_arr + arr_index);
        }

        const void* operator[](const size_t index) const {
            const auto arr_index = index * _BlockSize;
            return reinterpret_cast<void*>(this->m_arr + arr_index);
        }

        size_t calc_index_of(void* const p) const {
            const auto cp = reinterpret_cast<const uint8_t*>(p);
            return static_cast<size_t>(cp - &this->m_arr[0]) / _BlockSize;
        }

        void* data() {
            return reinterpret_cast<void*>(this->m_arr);
        }

        void* data() const {
            return reinterpret_cast<void*>(this->m_arr);
        }

    };


    template <typename _Typ, size_t _Size>
    class ArrayWithoutCtor {

    private:
        ObjectSizedBytesArray<sizeof(_Typ), _Size> m_array;
        std::array<bool, _Size> m_isConstrcuted = { false };

    public:
        ~ArrayWithoutCtor(void) {
            for (size_t i = 0; i < _Size; ++i) {
                this->destory_at(i);
            }
        }

        bool is_constructed(const size_t index) const {
            return this->m_isConstrcuted[index];
        }

        template <typename... VAL_TYPE>
        _Typ& construct_at(const size_t index, VAL_TYPE&&... params) {
            if (this->is_constructed(index))
                this->destory_at(index);

            const auto ptr = this->get_ptr_at(index);
            new (ptr) _Typ(std::forward<VAL_TYPE>(params)...);
            this->m_isConstrcuted[index] = true;
            return *ptr;
        }

        void destory_at(const size_t index) {
            if (this->is_constructed(index)) {
                const auto ptr = this->get_ptr_at(index);
                (*ptr).~_Typ();
                this->m_isConstrcuted[index] = false;
            }
        }

        _Typ& operator[](const size_t index) {
            return *this->get_ptr_at(index);
        }

        const _Typ& operator[](const size_t index) const {
            return *this->get_ptr_at(index);
        }

        _Typ& at(const size_t index) {
            return *this->get_ptr_at(index);
        }

        const _Typ& at(const size_t index) const {
            return *this->get_ptr_at(index);
        }

    private:
        _Typ* get_ptr_at(const size_t index) {
            return reinterpret_cast<_Typ*>(this->m_array[index]);
        }

        const _Typ* get_ptr_at(const size_t index) const {
            return reinterpret_cast<const _Typ*>(this->m_array[index]);
        }

    };


    template <typename T, size_t S>
    class StaticVector {

    public:
        ArrayWithoutCtor<T, S> m_data;
        size_t m_size = 0;

    public:
        StaticVector() = default;

        T& emplace_back() {
            return this->m_data.construct_at(this->m_size++);
        }

    };

}
