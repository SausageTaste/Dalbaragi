#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "d_defines.h"


namespace dal {

    class ResPath {

    private:
        static constexpr char SEPARATOR = '/';

    private:
        std::vector<std::string> m_dir_list;
        std::string m_src_str;

    public:
        ResPath() = default;

        ResPath(const char* const path);

        ResPath(const std::string& path)
            : ResPath(path.c_str())
        {

        }

        std::string make_str() const;

        bool is_valid() const;

        auto& dir_list() const {
            return this->m_dir_list;
        }

    };

}


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


    class AssetManager {

#ifdef DAL_OS_ANDROID
    private:
        void* m_ptr_asset_manager = nullptr;

    public:
        void set_android_asset_manager(void* const ptr_asset_manager) {
            this->m_ptr_asset_manager = ptr_asset_manager;
        }

        void* get_android_asset_manager() const {
            return this->m_ptr_asset_manager;
        }
#endif

    public:
        AssetManager() = default;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;

    public:
        std::vector<std::string> listfile(const dal::ResPath& path);

        std::unique_ptr<FileReadOnly> open(const dal::ResPath& path);

    };

}


namespace dal {

    class Filesystem {

    private:
        filesystem::AssetManager m_asset_man;

    public:
        auto& asset_mgr() {
            return this->m_asset_man;
        }

        [[nodiscard]]
        std::optional<dal::ResPath> resolve_respath(const dal::ResPath& respath);

        std::unique_ptr<dal::filesystem::FileReadOnly> open(const dal::ResPath& path);

    };

}
