#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "d_defines.h"


namespace dal::filesystem {

    class FileReadOnly {

    public:
        virtual ~FileReadOnly() = default;

        virtual bool open(const char* const path) = 0;

        virtual void close() = 0;

        virtual bool is_ready() = 0;

        virtual size_t size() = 0;

        virtual bool read(void* const dst, const size_t dst_size) = 0;

        template <typename T>
        bool read_stl(T& output) {
            if (!this->is_ready())
                return false;

            output.resize(this->size());
            return this->read(output.data(), output.size());
        }

        template <typename T>
        std::optional<T> read_stl() {
            T output{};

            if (!this->is_ready())
                return std::nullopt;

            output.resize(this->size());
            return this->read_stl(output) ? std::optional<T>{output} : std::nullopt;
        }

    };

}


namespace dal::filesystem {

    class AssetManager {

#ifdef DAL_OS_ANDROID
    private:
        void* m_ptr_asset_manager = nullptr;

    public:
        void set_android_asset_manager(void* const ptr_asset_manager) {
            this->m_ptr_asset_manager = ptr_asset_manager;
        }
#endif

    public:
        std::vector<std::string> listfile(const char* const path);

        std::unique_ptr<FileReadOnly> open(const char* const path);

    };

}
