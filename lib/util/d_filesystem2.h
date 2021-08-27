#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>


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


    class FileReadOnly {

    public:
        virtual ~FileReadOnly() = default;

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


    std::unique_ptr<FileReadOnly> make_file_read_only_null();


    class IFileManager {

    public:
        virtual bool is_file(const ResPath& path) = 0;

        virtual bool is_folder(const ResPath& path) = 0;

        virtual size_t list_files(const ResPath& path, std::vector<std::string>& output) = 0;

        virtual size_t list_folders(const ResPath& path, std::vector<std::string>& output) = 0;

        virtual std::optional<ResPath> resolve(const ResPath& path) = 0;

        virtual std::unique_ptr<FileReadOnly> open(const ResPath& path) = 0;

    };


    class IAssetManager : public IFileManager {

    };


    class IUserDataManager : public IFileManager {

    };


    class Filesystem2 {

    private:
        std::unique_ptr<IAssetManager> m_asset_mgr;
        std::unique_ptr<IUserDataManager> m_userdata_mgr;

    public:
        std::optional<ResPath> resolve_respath(const ResPath& path);

        std::unique_ptr<FileReadOnly> open(const ResPath& path);

    };

}
